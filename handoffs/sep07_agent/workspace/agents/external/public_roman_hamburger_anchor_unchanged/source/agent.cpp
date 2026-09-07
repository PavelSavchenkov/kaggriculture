#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>

#include "generated_actions.hpp"

namespace league::public_unchanged::roman_hamburger {
namespace {

static_assert(generated::SOURCE_SHA256 ==
    "00e8ab594a39779e808dd351d0973260f4c90ddc1370e684fa46eaa184ae4546");

constexpr std::array<int, kag::N_ITEMS> ITEM_NAME_RANK{
    7, 0, 6, 5, 3, 1, 4, 8, 2, 10, 9, 11,
};

const generated::TapeStep& tape_step(int step) {
    step = std::clamp(step, 0, static_cast<int>(generated::STEPS.size()) - 1);
    return generated::STEPS[step];
}

kag::UnitAction tape_unit(int step, int unit) {
    const generated::TapeStep& entry = tape_step(step);
    if (unit < 0 || unit >= entry.n_units) return {};
    return generated::UNITS[entry.unit_offset + unit];
}

void tape_action(int step, kag::Action& action) {
    action.clear();
    const generated::TapeStep& entry = tape_step(step);
    action.n_units = entry.n_units;
    action.n_orders = entry.n_orders;
    if (action.n_units > static_cast<int>(std::size(action.units)) ||
        action.n_orders > static_cast<int>(std::size(action.orders)))
        std::abort();
    std::copy_n(generated::UNITS.begin() + entry.unit_offset,
                action.n_units, action.units);
    std::copy_n(generated::ORDERS.begin() + entry.order_offset,
                action.n_orders, action.orders);
}

bool same_position(const kag::agent::PublicFarm& farm, int unit, int x, int y) {
    return unit >= 0 && unit < farm.n_units &&
        farm.pos_x[unit] == x && farm.pos_y[unit] == y;
}

bool is_empty(const kag::agent::PublicFarm& farm, int x, int y) {
    return x >= 0 && x < kag::BOARD && y >= 0 && y < kag::BOARD &&
        farm.tiles[y][x].kind == kag::T_EMPTY;
}

bool is_object(const kag::agent::PublicFarm& farm, int x, int y) {
    return x >= 0 && x < kag::BOARD && y >= 0 && y < kag::BOARD &&
        farm.tiles[y][x].kind != kag::T_EMPTY &&
        farm.tiles[y][x].kind != kag::T_LOCKED;
}

bool is_weed(const kag::agent::PublicFarm& farm, int unit) {
    if (unit < 0 || unit >= farm.n_units) return false;
    return farm.tiles[farm.pos_y[unit]][farm.pos_x[unit]].kind == kag::T_WEED;
}

int repair_rank(bool activated, int item) {
    if (!activated) return 0;
    if (item == kag::MILK) return 4;
    if (item == kag::FERTILIZER) return 3;
    if (item == kag::STRAWBERRY) return 2;
    if (item == kag::WHEAT) return 1;
    return 0;
}

struct ItemChoice {
    bool valid = false;
    int item = kag::N_ITEMS;
    int quantity = 0;
};

ItemChoice best_terminal_item(
    const kag::agent::AgentObservation& observation,
    const SeatState& state,
    int unit) {
    ItemChoice best{};
    std::tuple<int, long long, int, int, int> best_key{};
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const int quantity = std::max<int>(0, observation.own.inv[unit][item]);
        if (quantity <= 0) continue;
        const int price = std::max(0, observation.market.prices[item]);
        const auto key = std::make_tuple(
            repair_rank(state.soil_repair_activated, item),
            static_cast<long long>(price) * quantity,
            price,
            quantity,
            ITEM_NAME_RANK[item]);
        if (!best.valid || key > best_key) {
            best = {true, item, quantity};
            best_key = key;
        }
    }
    return best;
}

void terminal_agent(
    const kag::agent::AgentObservation& observation,
    const SeatState& state,
    int step,
    kag::Action& action) {
    tape_action(step, action);
    // Omitted live-hand entries are PASS and extra scheduled entries are
    // ignored by the Python game interpreter.
    action.n_units = observation.self().n_units;
    if (step < 716) return;
    const int n_units = observation.self().n_units;
    action.n_units = n_units;
    std::fill_n(action.units, n_units, kag::UnitAction{});
    std::array<int, kag::N_PRODUCTS> placed{};
    for (int unit = 0; unit < n_units; ++unit) {
        const ItemChoice choice = best_terminal_item(observation, state, unit);
        if (!choice.valid) continue;
        action.units[unit] = {
            kag::OP_PLACE, static_cast<uint8_t>(choice.item), choice.quantity};
        placed[choice.item] += choice.quantity;
    }
    struct Sale {
        int item;
        int quantity;
        std::tuple<int, long long, int, int, int> key;
    };
    std::array<Sale, kag::N_PRODUCTS> sales{};
    int n_sales = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const int quantity = std::max<int>(0, observation.own.shed[item]) +
            placed[item];
        if (quantity <= 0) continue;
        int market_rank = 0;
        if (state.soil_repair_activated) {
            if (item == kag::MILK) market_rank = 4;
            else if (item == kag::STRAWBERRY) market_rank = 3;
            else if (item == kag::FERTILIZER) market_rank = 2;
            else if (item == kag::WHEAT) market_rank = 1;
        }
        const int price = std::max(0, observation.market.prices[item]);
        sales[n_sales++] = {
            item,
            quantity,
            std::make_tuple(
                market_rank,
                static_cast<long long>(price) * quantity,
                price,
                quantity,
                ITEM_NAME_RANK[item]),
        };
    }
    std::sort(sales.begin(), sales.begin() + n_sales,
              [](const Sale& left, const Sale& right) {
        return left.key > right.key;
    });
    action.n_orders = std::min(10, n_sales);
    for (int index = 0; index < action.n_orders; ++index)
        action.orders[index] = {
            kag::M_SELL,
            static_cast<uint8_t>(sales[index].item),
            sales[index].quantity,
        };
}

void pasture_repair(
    const kag::agent::AgentObservation& observation,
    SeatState& state,
    int step,
    kag::Action& action) {
    terminal_agent(observation, state, step, action);
    if (step >= 716) return;
    const kag::agent::PublicFarm& farm = observation.self();
    if (state.farmer_shift_end >= 0) {
        if (step <= state.farmer_shift_end) action.units[0] = tape_unit(step - 1, 0);
        else state.farmer_shift_end = -1;
    }
    if (state.pasture.active) {
        const PendingPasture pending = state.pasture;
        if (step == pending.expected_step && is_empty(farm, pending.x, pending.y)) {
            if (pending.farmer && same_position(farm, 0, pending.x, pending.y)) {
                action.units[0] = {kag::OP_BUILD_PASTURE, kag::N_ITEMS, 1};
            } else if (!pending.farmer &&
                       same_position(farm, pending.actor + 1, pending.x, pending.y) &&
                       pending.actor + 1 < action.n_units) {
                action.units[pending.actor + 1] = {
                    kag::OP_BUILD_PASTURE, kag::N_ITEMS, 1};
            }
        }
        state.pasture = {};
    }
    if (action.n_units > 0 && action.units[0].op == kag::OP_BUILD_PASTURE &&
        is_weed(farm, 0)) {
        action.units[0] = {kag::OP_DIG, kag::N_ITEMS, 1};
        if (step % 24 >= 20) state.farmer_shift_end = (step / 24 + 1) * 24 - 1;
        state.pasture = {
            true, true, -1, farm.pos_x[0], farm.pos_y[0], step + 1};
    }
    for (int unit = 1; unit < std::min(action.n_units, farm.n_units); ++unit) {
        if (state.pasture.active) break;
        if (action.units[unit].op != kag::OP_BUILD_PASTURE || !is_weed(farm, unit))
            continue;
        action.units[unit] = {kag::OP_DIG, kag::N_ITEMS, 1};
        state.pasture = {
            true, false, unit - 1,
            farm.pos_x[unit], farm.pos_y[unit], step + 1};
    }
}

void soil_repair(
    const kag::agent::AgentObservation& observation,
    SeatState& state,
    int step,
    kag::Action& action) {
    pasture_repair(observation, state, step, action);
    if (step >= 716) return;
    const kag::agent::PublicFarm& farm = observation.self();
    if (state.water_shift_actor >= 0) {
        const int unit = state.water_shift_actor + 1;
        if (step <= state.water_shift_end && unit < action.n_units) {
            action.units[unit] = tape_unit(step - 1, unit);
        } else {
            state.water_shift_actor = -1;
            state.water_shift_end = -1;
        }
    }
    if (state.water.active) {
        const PendingWater pending = state.water;
        if (step == pending.expected_step && is_object(farm, pending.x, pending.y)) {
            int actor = -1;
            for (int unit = 1; unit < farm.n_units; ++unit) {
                if (unit - 1 != pending.planter && unit < action.n_units &&
                    same_position(farm, unit, pending.x, pending.y)) {
                    actor = unit - 1;
                    break;
                }
            }
            if (actor < 0 && pending.planter + 1 < action.n_units)
                actor = pending.planter;
            if (actor >= 0) {
                action.units[actor + 1] = {kag::OP_WATER, kag::N_ITEMS, 1};
                state.water_shift_actor = actor;
                state.water_shift_end = (step / 24 + 1) * 24 - 1;
            }
        }
        state.water = {};
    }
    if (state.plant.active) {
        const PendingPlant pending = state.plant;
        const int unit = pending.actor + 1;
        if (step == pending.expected_step && unit < farm.n_units &&
            unit < action.n_units &&
            same_position(farm, unit, pending.x, pending.y) &&
            is_empty(farm, pending.x, pending.y) &&
            pending.crop < kag::N_CROPS &&
            observation.own.seeds[pending.crop] > 0) {
            action.units[unit] = {
                kag::OP_PLANT, static_cast<uint8_t>(pending.crop), 1};
            state.water = {
                true, pending.actor, pending.x, pending.y, step + 1};
        }
        state.plant = {};
    }
    if (step != 636) return;
    for (int unit = 1; unit < std::min(action.n_units, farm.n_units); ++unit) {
        if (action.units[unit].op != kag::OP_PLANT ||
            action.units[unit].arg != kag::WHEAT || !is_weed(farm, unit))
            continue;
        action.units[unit] = {kag::OP_DIG, kag::N_ITEMS, 1};
        state.plant = {
            true, unit - 1, kag::WHEAT,
            farm.pos_x[unit], farm.pos_y[unit], step + 1};
        state.soil_repair_activated = true;
        break;
    }
}

int cashflow_rank(const kag::Order& order) {
    if (order.op == kag::M_SELL) {
        if (order.item == kag::WOOL) return 0;
        if (order.item == kag::MELON) return 1;
        if (order.item == kag::MILK) return 2;
        if (order.item == kag::STRAWBERRY) return 3;
        if (order.item == kag::CARROT) return 4;
        if (order.item == kag::FERTILIZER) return 5;
        if (order.item == kag::WHEAT) return 6;
        if (order.item == kag::EGG) return 7;
        return 16;
    }
    if (order.op == kag::M_HIRE) return 8;
    if (order.op == kag::M_BUY_ANIMAL && order.item == kag::COW) return 9;
    if (order.op == kag::M_BUY_ANIMAL && order.item == kag::SHEEP) return 10;
    if (order.op == kag::M_BUY_LAND) return 11;
    if (order.op == kag::M_BUY_SEED && order.item == kag::MELON) return 12;
    if (order.op == kag::M_BUY_SEED && order.item == kag::STRAWBERRY) return 13;
    if (order.op == kag::M_BUY_SEED && order.item == kag::WHEAT) return 14;
    if (order.op == kag::M_BUY_PRODUCT && order.item == kag::WHEAT) return 15;
    return 16;
}

void cashflow_agent(
    const kag::agent::AgentObservation& observation,
    SeatState& state,
    int step,
    kag::Action& action) {
    soil_repair(observation, state, step, action);
    if (step == 300) {
        std::array<int, kag::N_ITEMS> counts{};
        for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
            const kag::Tile& tile = observation.opponent().tiles[y][x];
            if (tile.has_animal && tile.what < kag::N_ITEMS) ++counts[tile.what];
            else if (tile.kind == kag::T_PLANT && tile.what < kag::N_ITEMS)
                ++counts[tile.what];
        }
        state.cashflow_active =
            counts[kag::WHEAT] >= 5 && counts[kag::STRAWBERRY] >= 26 &&
            counts[kag::MELON] == 6 && counts[kag::COW] >= 8 &&
            counts[kag::SHEEP] >= 6;
    }
    if (!state.cashflow_active || step < 300 || step >= 715) return;
    std::stable_sort(action.orders, action.orders + action.n_orders,
        [&](const kag::Order& left, const kag::Order& right) {
            const int left_rank = cashflow_rank(left);
            const int right_rank = cashflow_rank(right);
            const bool left_sell = left.op == kag::M_SELL;
            const bool right_sell = right.op == kag::M_SELL;
            const auto left_key = std::make_tuple(
                left_sell ? 0 : left_rank,
                left_sell ? -static_cast<long long>(
                    std::max(0, observation.market.prices[left.item])) *
                    std::max(0, left.n) : 0LL,
                left_rank);
            const auto right_key = std::make_tuple(
                right_sell ? 0 : right_rank,
                right_sell ? -static_cast<long long>(
                    std::max(0, observation.market.prices[right.item])) *
                    std::max(0, right.n) : 0LL,
                right_rank);
            return left_key < right_key;
        });
}

}

kag::agent::AgentInfo Policy::info() {
    return {"public-roman-hamburger-anchor-exact-port"};
}

void Policy::reset(const kag::agent::AgentInit&) {
    seats_ = {};
}

void Policy::act(
    const kag::agent::AgentObservation& observation,
    const kag::agent::DecisionBudget&,
    kag::Action& action) {
    if (observation.player > 1 || observation.self().n_units > kag::MAX_UNITS)
        std::abort();
    const int step = std::clamp(observation.step, 0, 719);
    SeatState& state = seats_[observation.player];
    if (step == 0) state = {};
    cashflow_agent(observation, state, step, action);
    action.finalize();
}

}
