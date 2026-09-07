#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza {

struct GuardStats {
    std::array<int, kag::OP_INVALID + 1> unit_removed_by_op{};
    std::array<int, kag::M_SELL + 1> order_units_removed_by_op{};
    int unit_actions_removed = 0;
    int order_units_removed = 0;
    int lossy_drops_removed = 0;
    int dayend_sales_added = 0;
    int dayend_fertilizers_substituted = 0;
    int dayend_overflow_unresolved = 0;
};

namespace guard_detail {

struct Shadow {
    kag::Tile tiles[kag::BOARD][kag::BOARD]{};
    int8_t x[kag::MAX_UNITS]{};
    int8_t y[kag::MAX_UNITS]{};
    kag::Count shed[kag::N_ITEMS]{};
    kag::Count inv[kag::MAX_UNITS][kag::N_ITEMS]{};
    uint8_t inv_keys[kag::MAX_UNITS][kag::N_ITEMS]{};
    uint8_t inv_nkeys[kag::MAX_UNITS]{};
    kag::Count seeds[kag::N_CROPS]{};
    int shed_total = 0;
    int day = 0;
};

inline Shadow make_shadow(
    const kag::agent::AgentObservation& observation
) {
    Shadow shadow;
    shadow.day = observation.day;
    shadow.shed_total = observation.own.shed_total;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            shadow.tiles[y][x] = observation.self().tiles[y][x];
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        shadow.x[unit] = observation.self().pos_x[unit];
        shadow.y[unit] = observation.self().pos_y[unit];
        shadow.inv_nkeys[unit] = observation.own.inv_nkeys[unit];
        for (int item = 0; item < kag::N_ITEMS; ++item)
            shadow.inv[unit][item] = observation.own.inv[unit][item];
        for (int key = 0; key < shadow.inv_nkeys[unit]; ++key)
            shadow.inv_keys[unit][key] = observation.own.inv_keys[unit][key];
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        shadow.shed[item] = observation.own.shed[item];
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        shadow.seeds[crop] = observation.own.seeds[crop];
    return shadow;
}

inline void inv_add(Shadow& shadow, int unit, int item, int amount) {
    if (amount <= 0) return;
    if (!shadow.inv[unit][item])
        shadow.inv_keys[unit][shadow.inv_nkeys[unit]++] =
            static_cast<uint8_t>(item);
    shadow.inv[unit][item] = static_cast<kag::Count>(
        shadow.inv[unit][item] + amount);
}

inline void inv_take(Shadow& shadow, int unit, int item, int amount) {
    shadow.inv[unit][item] = static_cast<kag::Count>(
        shadow.inv[unit][item] - amount);
    if (shadow.inv[unit][item]) return;
    int key = 0;
    while (key < shadow.inv_nkeys[unit] &&
           shadow.inv_keys[unit][key] != item)
        ++key;
    if (key == shadow.inv_nkeys[unit]) return;
    for (; key + 1 < shadow.inv_nkeys[unit]; ++key)
        shadow.inv_keys[unit][key] = shadow.inv_keys[unit][key + 1];
    --shadow.inv_nkeys[unit];
}

inline bool movement_legal(const Shadow& shadow, int unit, uint8_t op) {
    const int nx = shadow.x[unit] + (op == kag::OP_EAST) -
        (op == kag::OP_WEST);
    const int ny = shadow.y[unit] + (op == kag::OP_SOUTH) -
        (op == kag::OP_NORTH);
    return nx >= 0 && nx < kag::BOARD && ny >= 0 && ny < kag::BOARD;
}

inline int inventory_total(const Shadow& shadow, int unit) {
    int result = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        result += shadow.inv[unit][item];
    return result;
}

inline bool executable(
    const Shadow& shadow,
    int unit,
    const kag::UnitAction& action
) {
    const int x = shadow.x[unit];
    const int y = shadow.y[unit];
    const kag::Tile& tile = shadow.tiles[y][x];
    switch (action.op) {
        case kag::OP_PASS: return true;
        case kag::OP_NORTH:
        case kag::OP_SOUTH:
        case kag::OP_EAST:
        case kag::OP_WEST:
            return movement_legal(shadow, unit, action.op);
        case kag::OP_PICKUP:
            return kag::is_shed_adjacent(x, y, kag::BOARD) &&
                action.arg < kag::N_ITEMS && action.n > 0 &&
                shadow.shed[action.arg] > 0;
        case kag::OP_DROP:
            return kag::is_shed_adjacent(x, y, kag::BOARD) &&
                inventory_total(shadow, unit) > 0;
        case kag::OP_PLACE: {
            if (action.arg >= kag::N_ITEMS || action.n <= 0) return false;
            if (kag::is_animal(action.arg)) {
                const auto structure =
                    kag::ANIMALS[action.arg - kag::GOOSE].structure;
                const kag::TileKind wanted = structure == kag::ST_COOP ?
                    kag::T_COOP : kag::T_PASTURE;
                if (tile.kind == wanted && !tile.has_animal)
                    return shadow.inv[unit][action.arg] > 0;
            }
            return kag::is_shed_adjacent(x, y, kag::BOARD) &&
                shadow.inv[unit][action.arg] > 0 &&
                shadow.shed_total < 100;
        }
        case kag::OP_PLANT:
            return action.arg < kag::N_CROPS &&
                tile.kind == kag::T_EMPTY && shadow.seeds[action.arg] > 0;
        case kag::OP_WATER:
            return tile.kind == kag::T_PLANT && !tile.watered_today;
        case kag::OP_HARVEST:
            if (tile.yield_units <= 0) return false;
            if (tile.kind == kag::T_PLANT)
                return shadow.day - tile.planted_day >=
                    kag::CROPS[tile.what].first_yield_day;
            return tile.has_animal;
        case kag::OP_FERTILIZE:
            return tile.kind == kag::T_PLANT &&
                shadow.inv[unit][kag::FERTILIZER] > 0;
        case kag::OP_DIG:
            return tile.kind != kag::T_EMPTY &&
                tile.kind != kag::T_LOCKED && !tile.has_animal;
        case kag::OP_BUILD_COOP:
        case kag::OP_BUILD_PASTURE:
            return tile.kind == kag::T_EMPTY;
        case kag::OP_FEED:
            return tile.has_animal && !tile.fed_today &&
                shadow.inv[unit][kag::WHEAT] > 0;
        case kag::OP_COLLECT_FERTILIZER:
            return tile.has_animal && tile.fertilizer_available;
        case kag::OP_CARE:
            return tile.has_animal && !tile.cared_today;
        default: return false;
    }
}

inline void apply(Shadow& shadow, int unit, const kag::UnitAction& action) {
    int x = shadow.x[unit];
    int y = shadow.y[unit];
    if (action.op >= kag::OP_NORTH && action.op <= kag::OP_WEST) {
        x += (action.op == kag::OP_EAST) - (action.op == kag::OP_WEST);
        y += (action.op == kag::OP_SOUTH) - (action.op == kag::OP_NORTH);
        shadow.x[unit] = static_cast<int8_t>(x);
        shadow.y[unit] = static_cast<int8_t>(y);
        return;
    }
    kag::Tile& tile = shadow.tiles[y][x];
    switch (action.op) {
        case kag::OP_PICKUP: {
            const int amount =
                std::min<int>(action.n, shadow.shed[action.arg]);
            shadow.shed[action.arg] = static_cast<kag::Count>(
                shadow.shed[action.arg] - amount);
            shadow.shed_total -= amount;
            inv_add(shadow, unit, action.arg, amount);
            break;
        }
        case kag::OP_DROP: {
            const int keys = shadow.inv_nkeys[unit];
            for (int key = 0; key < keys; ++key) {
                const int item = shadow.inv_keys[unit][key];
                const int amount = std::min<int>(
                    shadow.inv[unit][item], 100 - shadow.shed_total);
                shadow.shed[item] = static_cast<kag::Count>(
                    shadow.shed[item] + amount);
                shadow.shed_total += amount;
                shadow.inv[unit][item] = 0;
            }
            shadow.inv_nkeys[unit] = 0;
            break;
        }
        case kag::OP_PLACE: {
            if (kag::is_animal(action.arg)) {
                const auto structure =
                    kag::ANIMALS[action.arg - kag::GOOSE].structure;
                const kag::TileKind wanted = structure == kag::ST_COOP ?
                    kag::T_COOP : kag::T_PASTURE;
                if (tile.kind == wanted && !tile.has_animal) {
                    inv_take(shadow, unit, action.arg, 1);
                    tile = kag::Tile{};
                    tile.kind = wanted;
                    tile.what = action.arg;
                    tile.has_animal = true;
                    tile.planted_day = static_cast<int16_t>(shadow.day);
                    break;
                }
            }
            const int amount = std::min({
                static_cast<int>(action.n),
                static_cast<int>(shadow.inv[unit][action.arg]),
                100 - shadow.shed_total});
            inv_take(shadow, unit, action.arg, amount);
            shadow.shed[action.arg] = static_cast<kag::Count>(
                shadow.shed[action.arg] + amount);
            shadow.shed_total += amount;
            break;
        }
        case kag::OP_PLANT: {
            --shadow.seeds[action.arg];
            kag::Tile planted{};
            planted.kind = kag::T_PLANT;
            planted.what = action.arg;
            planted.planted_day = static_cast<int16_t>(shadow.day);
            planted.consecutive_dry = 1;
            planted.yield_units = kag::CROPS[action.arg].ongoing ? 0 : 1;
            tile = planted;
            break;
        }
        case kag::OP_WATER: {
            tile.watered_today = true;
            const auto& crop = kag::CROPS[tile.what];
            const int age = shadow.day - tile.planted_day;
            const int bonus_age = (crop.max_yield_day + 1) / 2;
            if (!crop.ongoing && age >= bonus_age &&
                age <= crop.max_yield_day) {
                const int bonus = tile.fertilized_until_day >= shadow.day ?
                    2 : 1;
                tile.yield_units = static_cast<int8_t>(std::min<int>(
                    crop.max_yield, tile.yield_units + bonus));
            }
            break;
        }
        case kag::OP_HARVEST: {
            const int item = tile.kind == kag::T_PLANT ?
                static_cast<int>(tile.what) :
                static_cast<int>(
                    kag::ANIMALS[tile.what - kag::GOOSE].product);
            inv_add(shadow, unit, item, tile.yield_units);
            if (tile.kind == kag::T_PLANT &&
                !kag::CROPS[tile.what].ongoing)
                tile = kag::Tile{};
            else
                tile.yield_units = 0;
            break;
        }
        case kag::OP_FERTILIZE:
            inv_take(shadow, unit, kag::FERTILIZER, 1);
            tile.fertilized_until_day = std::max<int16_t>(
                tile.fertilized_until_day,
                static_cast<int16_t>(shadow.day + 2));
            break;
        case kag::OP_DIG:
            tile = kag::Tile{};
            break;
        case kag::OP_BUILD_COOP:
            tile = kag::Tile{};
            tile.kind = kag::T_COOP;
            break;
        case kag::OP_BUILD_PASTURE:
            tile = kag::Tile{};
            tile.kind = kag::T_PASTURE;
            break;
        case kag::OP_FEED:
            inv_take(shadow, unit, kag::WHEAT, 1);
            tile.fed_today = true;
            break;
        case kag::OP_COLLECT_FERTILIZER:
            tile.fertilizer_available = false;
            inv_add(shadow, unit, kag::FERTILIZER, 1);
            break;
        case kag::OP_CARE:
            tile.cared_today = true;
            break;
        default: break;
    }
}

inline int order_units(const kag::Order& order) {
    if (order.op == kag::M_HIRE || order.op == kag::M_BUY_LAND) return 1;
    return std::max(0, order.n);
}

}  // namespace guard_detail

inline GuardStats guard_observation(
    const kag::agent::AgentObservation& observation,
    kag::Action& action
) {
    GuardStats stats;
    auto shadow = guard_detail::make_shadow(observation);
    const int units = observation.self().n_units;
    for (int unit = std::max(0, action.n_units); unit < units; ++unit)
        action.units[unit] = {};
    action.n_units = units;
    for (int unit = 0; unit < units; ++unit) {
        kag::UnitAction& move = action.units[unit];
        bool keep = guard_detail::executable(shadow, unit, move);
        if (keep && move.op == kag::OP_DROP &&
            guard_detail::inventory_total(shadow, unit) >
                100 - shadow.shed_total) {
            keep = false;
            ++stats.lossy_drops_removed;
        }
        if (!keep) {
            if (move.op <= kag::OP_INVALID)
                ++stats.unit_removed_by_op[move.op];
            ++stats.unit_actions_removed;
            move = {};
            continue;
        }
        guard_detail::apply(shadow, unit, move);
    }
    action.finalize();

    double money = observation.self().money;
    int hires = observation.self().hires_today;
    int n_units = observation.self().n_units;
    int quadrants = observation.self().n_quadrants;
    std::array<int, kag::N_PRODUCTS> market{};
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        market[item] = observation.market.inventory[item];

    int write = 0;
    const int count = std::clamp(action.n_orders, 0, 10);
    for (int index = 0; index < count; ++index) {
        kag::Order order = action.orders[index];
        const int requested = guard_detail::order_units(order);
        int accepted = 0;
        if (order.op == kag::M_HIRE) {
            const int price = kag::fib(hires);
            if (money >= price && n_units < kag::MAX_UNITS) {
                money -= price;
                ++hires;
                ++n_units;
                accepted = 1;
            }
        } else if (order.op == kag::M_BUY_LAND) {
            if (quadrants < 4) {
                const int price = kag::LAND_PRICES[quadrants - 1];
                if (money >= price) {
                    money -= price;
                    ++quadrants;
                    accepted = 1;
                }
            }
        } else if (order.n > 0) {
            while (accepted < order.n) {
                if (order.op == kag::M_SELL &&
                    order.item < kag::N_PRODUCTS &&
                    shadow.shed[order.item] > 0) {
                    const int price = kag::market_price(
                        order.item, market[order.item]);
                    money += price;
                    --shadow.shed[order.item];
                    --shadow.shed_total;
                    market[order.item] += price > 1;
                } else if (order.op == kag::M_BUY_PRODUCT &&
                           (order.item == kag::WHEAT ||
                            order.item == kag::FERTILIZER) &&
                           shadow.shed_total < 100) {
                    const int price = kag::market_price(
                        order.item, market[order.item] - 1);
                    if (money < price) break;
                    money -= price;
                    --market[order.item];
                    ++shadow.shed[order.item];
                    ++shadow.shed_total;
                } else if (order.op == kag::M_BUY_SEED &&
                           order.item < kag::N_CROPS) {
                    const int price = kag::CROPS[order.item].seed;
                    if (money < price) break;
                    money -= price;
                } else if (order.op == kag::M_BUY_ANIMAL &&
                           kag::is_animal(order.item) &&
                           shadow.shed_total < 100) {
                    const int price =
                        kag::ANIMALS[order.item - kag::GOOSE].cost;
                    if (money < price) break;
                    money -= price;
                    ++shadow.shed[order.item];
                    ++shadow.shed_total;
                } else {
                    break;
                }
                ++accepted;
            }
        }
        const int removed = requested - accepted;
        if (order.op <= kag::M_SELL)
            stats.order_units_removed_by_op[order.op] += removed;
        stats.order_units_removed += removed;
        if (accepted <= 0) continue;
        if (order.op != kag::M_HIRE && order.op != kag::M_BUY_LAND)
            order.n = accepted;
        action.orders[write++] = order;
    }
    if (observation.hour == 23) {
        int carried = 0;
        for (int unit = 0; unit < n_units; ++unit)
            carried += guard_detail::inventory_total(shadow, unit);
        int overflow = std::max(0, shadow.shed_total + carried - 100);
        while (overflow > 0 && write < 10) {
            int best_item = -1;
            int best_price = -1;
            for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                if (shadow.shed[item] <= 0) continue;
                const int price = kag::market_price(item, market[item]);
                if (price > best_price) {
                    best_item = item;
                    best_price = price;
                }
            }
            if (best_item < 0) break;
            const int amount = std::min<int>(overflow,
                                              shadow.shed[best_item]);
            action.orders[write++] = {
                kag::M_SELL, static_cast<uint8_t>(best_item), amount};
            shadow.shed[best_item] = static_cast<kag::Count>(
                shadow.shed[best_item] - amount);
            shadow.shed_total -= amount;
            for (int unit = 0; unit < amount; ++unit) {
                const int price = kag::market_price(best_item,
                                                     market[best_item]);
                market[best_item] += price > 1;
            }
            overflow -= amount;
            stats.dayend_sales_added += amount;
        }
        stats.dayend_overflow_unresolved = overflow;
        if (overflow > 0) {
            int best_unit = -1;
            int best_amount = 1000000;
            int best_loss = 1000000000;
            uint8_t best_op = kag::OP_PASS;
            for (int unit = 0; unit < n_units; ++unit) {
                const auto& move = action.units[unit];
                int amount = 0;
                int item = -1;
                if (move.op == kag::OP_COLLECT_FERTILIZER) {
                    amount = 1;
                    item = kag::FERTILIZER;
                } else if (move.op == kag::OP_HARVEST) {
                    const int x = observation.self().pos_x[unit];
                    const int y = observation.self().pos_y[unit];
                    const auto& tile = observation.self().tiles[y][x];
                    amount = tile.yield_units;
                    item = tile.kind == kag::T_PLANT ?
                        static_cast<int>(tile.what) :
                        static_cast<int>(kag::ANIMALS[
                            tile.what - kag::GOOSE].product);
                }
                int loss = 0;
                int inventory = item >= 0 && item < kag::N_PRODUCTS ?
                    observation.market.inventory[item] : 0;
                for (int index = 0; index < amount; ++index) {
                    const int price = kag::market_price(item, inventory);
                    loss += price;
                    inventory += price > 1;
                }
                if (amount > 0 &&
                    (loss < best_loss ||
                     (loss == best_loss && amount < best_amount))) {
                    best_unit = unit;
                    best_amount = amount;
                    best_loss = loss;
                    best_op = move.op;
                }
            }
            int fertilize_unit = -1;
            int fertilize_loss = 1000000000;
            for (int unit = 0; unit < n_units; ++unit) {
                const auto& old = action.units[unit];
                if (old.op != kag::OP_PASS && old.op != kag::OP_WATER)
                    continue;
                const int x = observation.self().pos_x[unit];
                const int y = observation.self().pos_y[unit];
                const auto& tile = observation.self().tiles[y][x];
                if (tile.kind != kag::T_PLANT ||
                    kag::CROPS[tile.what].ongoing ||
                    observation.own.inv[unit][kag::FERTILIZER] <= 0)
                    continue;
                const kag::UnitAction proposal{
                    kag::OP_FERTILIZE, 0, 1};
                if (!guard_detail::executable(shadow, unit, proposal))
                    continue;
                int loss = kag::market_price(
                    kag::FERTILIZER,
                    observation.market.inventory[kag::FERTILIZER]);
                if (old.op == kag::OP_WATER && !tile.watered_today) {
                    const auto& crop = kag::CROPS[tile.what];
                    const int age = observation.day - tile.planted_day;
                    const int bonus_age = (crop.max_yield_day + 1) / 2;
                    int lost_units = 0;
                    if (age >= bonus_age && age <= crop.max_yield_day) {
                        const int bonus =
                            tile.fertilized_until_day >= observation.day ? 2 : 1;
                        lost_units = std::min<int>(
                            bonus, crop.max_yield - tile.yield_units);
                    }
                    int inventory = observation.market.inventory[tile.what];
                    for (int index = 0; index < lost_units; ++index) {
                        const int price = kag::market_price(tile.what, inventory);
                        loss += price;
                        inventory += price > 1;
                    }
                }
                if (loss < fertilize_loss) {
                    fertilize_loss = loss;
                    fertilize_unit = unit;
                }
            }
            if (fertilize_unit >= 0 && fertilize_loss < best_loss) {
                action.units[fertilize_unit] = {
                    kag::OP_FERTILIZE, 0, 1};
                action.n_orders = write;
                action.finalize();
                GuardStats retry = guard_observation(observation, action);
                ++retry.dayend_fertilizers_substituted;
                retry.lossy_drops_removed += stats.lossy_drops_removed;
                retry.order_units_removed += stats.order_units_removed;
                for (int op = 0; op <= kag::M_SELL; ++op)
                    retry.order_units_removed_by_op[op] +=
                        stats.order_units_removed_by_op[op];
                return retry;
            }
            if (best_unit >= 0) {
                action.units[best_unit] = {};
                action.n_orders = write;
                action.finalize();
                GuardStats retry = guard_observation(observation, action);
                ++retry.unit_actions_removed;
                ++retry.unit_removed_by_op[best_op];
                retry.lossy_drops_removed += stats.lossy_drops_removed;
                retry.order_units_removed += stats.order_units_removed;
                for (int op = 0; op <= kag::M_SELL; ++op)
                    retry.order_units_removed_by_op[op] +=
                        stats.order_units_removed_by_op[op];
                return retry;
            }
        }
    }
    action.n_orders = write;
    action.finalize();
    return stats;
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::pizza
