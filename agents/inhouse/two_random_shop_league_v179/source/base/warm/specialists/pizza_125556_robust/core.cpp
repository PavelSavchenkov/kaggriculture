#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pizza_125556_robust/core.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <limits>

namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::detail {

#ifndef FIXED_WEED_TAPE_INCLUDE
#define FIXED_WEED_TAPE_INCLUDE \
    "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pizza_125556_robust/tape.inc"
#endif
#include FIXED_WEED_TAPE_INCLUDE

namespace {

kag::UnitAction nominal_unit(int step, int wanted_unit) {
    if (step < 0 || step >= TAPE_STEPS) return {};
    int cursor = TAPE_OFFSETS[step];
    const int units = TAPE_DATA[cursor++];
    ++cursor;
    if (wanted_unit < 0 || wanted_unit >= units) return {};
    cursor += 3 * wanted_unit;
    return {
        static_cast<uint8_t>(TAPE_DATA[cursor]),
        static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
        TAPE_DATA[cursor + 2],
    };
}

int nominal_units(int step) {
    return step >= 0 && step < TAPE_STEPS ? TAPE_DATA[TAPE_OFFSETS[step]] : 1;
}

void nominal_orders(int step, kag::Action& action) {
    if (step < 0 || step >= TAPE_STEPS) return;
    int cursor = TAPE_OFFSETS[step];
    const int units = TAPE_DATA[cursor++];
    const int orders = TAPE_DATA[cursor++];
    cursor += 3 * units;
    action.n_orders = std::min(orders, 10);
    for (int order = 0; order < action.n_orders; ++order) {
        action.orders[order] = {
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2],
        };
        cursor += 3;
    }
}

kag::Action nominal_action(int step, int live_units) {
    kag::Action action;
    action.clear();
    action.n_units = live_units;
    for (int unit = 0; unit < live_units; ++unit)
        action.units[unit] = nominal_unit(step, unit);
    nominal_orders(step, action);
    action.finalize();
    return action;
}

struct Commitment {
    int16_t step = 0;
    int8_t x = 0;
    int8_t y = 0;
    uint8_t op = 0;
    uint8_t arg = 0;
};

struct NominalTable {
    std::array<Commitment, 256> commitments{};
    int commitment_count = 0;
    std::array<int16_t, 30> last_hire_step{};
    std::array<std::array<int32_t, kag::N_PRODUCTS>, TAPE_STEPS> sold_through_step{};
    std::array<std::array<int16_t, kag::N_PRODUCTS>, TAPE_STEPS> sale_by_step{};
    std::array<std::array<int16_t, kag::N_PRODUCTS>, TAPE_STEPS> owned_before_step{};
};

const NominalTable& nominal_table() {
    static const NominalTable table = [] {
        NominalTable result;
        result.last_hire_step.fill(-1);
        kag::Config config;
        config.seed = 1;
        config.shop_unlock_interval = 1000;
        config.weed_chance = 0;
        kag::Sim sim(config);
        kag::Action pass;
        pass.clear();
        pass.finalize();
        for (int step = 0; step < TAPE_STEPS; ++step) {
            for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                int owned = sim.st.farms[0].shed[item];
                for (int unit = 0; unit < sim.st.farms[0].n_units; ++unit)
                    owned += sim.st.farms[0].inv[unit][item];
                result.owned_before_step[step][item] = static_cast<int16_t>(owned);
            }
            const kag::Action action = nominal_action(step, sim.st.farms[0].n_units);
            for (int unit = 0; unit < sim.st.farms[0].n_units; ++unit) {
                const uint8_t op = action.units[unit].op;
                if (op != kag::OP_PLANT && op != kag::OP_BUILD_COOP &&
                    op != kag::OP_BUILD_PASTURE)
                    continue;
                if (result.commitment_count >= static_cast<int>(result.commitments.size()))
                    std::abort();
                result.commitments[result.commitment_count++] = {
                    static_cast<int16_t>(step), sim.st.farms[0].pos_x[unit],
                    sim.st.farms[0].pos_y[unit], op, action.units[unit].arg};
            }
            for (int order = 0; order < action.n_orders; ++order)
                if (action.orders[order].op == kag::M_HIRE)
                    result.last_hire_step[step / 24] = static_cast<int16_t>(step);
                else if (action.orders[order].op == kag::M_SELL &&
                         action.orders[order].item < kag::N_PRODUCTS)
                    result.sale_by_step[step][action.orders[order].item] =
                        static_cast<int16_t>(
                            result.sale_by_step[step][action.orders[order].item] +
                            action.orders[order].n);
            sim.step(action, pass);
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                result.sold_through_step[step][item] = sim.st.farms[0].sold_units[item];
        }
        return result;
    }();
    return table;
}

double shifted_sale_revenue_delta(int step, int item, int quantity,
                                  int current_inventory, double early_proceeds) {
    const NominalTable& table = nominal_table();
    std::array<int16_t, TAPE_STEPS> removed{};
    int remaining = quantity;
    for (int future = TAPE_STEPS - 1; future > step && remaining > 0; --future) {
        const int take = std::min<int>(remaining, table.sale_by_step[future][item]);
        removed[future] = static_cast<int16_t>(take);
        remaining -= take;
    }
    if (remaining > 0) return -1e12;

    int baseline_inventory = current_inventory;
    int shifted_inventory = current_inventory + quantity;
    double baseline_revenue = 0;
    double shifted_revenue = 0;
    for (int future = step + 1; future < TAPE_STEPS; ++future) {
        const int baseline_quantity = table.sale_by_step[future][item];
        const int shifted_quantity = baseline_quantity - removed[future];
        for (int unit = 0; unit < baseline_quantity; ++unit) {
            const int price = kag::market_price(item, baseline_inventory);
            baseline_revenue += price;
            baseline_inventory += price > 1;
        }
        for (int unit = 0; unit < shifted_quantity; ++unit) {
            const int price = kag::market_price(item, shifted_inventory);
            shifted_revenue += price;
            shifted_inventory += price > 1;
        }
        if (future % 24 == 0 && item < kag::FERTILIZER) {
            --baseline_inventory;
            --shifted_inventory;
        }
    }
    return early_proceeds + shifted_revenue - baseline_revenue;
}

bool shed_adjacent(int x, int y) {
    return kag::is_shed_adjacent(x, y, kag::BOARD);
}

std::array<int, 2> next_spawn(
    const kag::agent::AgentObservation& observation,
    int accepted_hires
) {
    int access[4][2];
    kag::shed_access_tiles(kag::BOARD, access);
    int occupancy[4]{};
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int tile = 0; tile < 4; ++tile)
            if (observation.self().pos_x[unit] == access[tile][0] &&
                observation.self().pos_y[unit] == access[tile][1])
                ++occupancy[tile];
    int spawn = 0;
    for (int hire = 0; hire <= accepted_hires; ++hire) {
        spawn = 0;
        for (int tile = 1; tile < 4; ++tile)
            if (occupancy[tile] < occupancy[spawn]) spawn = tile;
        ++occupancy[spawn];
    }
    return {access[spawn][0], access[spawn][1]};
}

uint8_t cleanup_weight(const Commitment& commitment) {
    if (commitment.op == kag::OP_BUILD_COOP || commitment.op == kag::OP_BUILD_PASTURE)
        return 80;
    static constexpr std::array<uint8_t, kag::N_CROPS> weights = {6, 4, 12, 30, 50};
    return commitment.arg < weights.size() ? weights[commitment.arg] : 1;
}

bool positive_cleanup_counterfactual(const Commitment& commitment) {
    switch (commitment.step) {
        case 134: return commitment.op == kag::OP_BUILD_COOP;
        case 273: case 283: case 286: case 329: case 476:
            return commitment.op == kag::OP_PLANT && commitment.arg == kag::WHEAT;
        case 276:
            return commitment.op == kag::OP_PLANT && commitment.arg == kag::WHEAT;
        case 280:
            return commitment.op == kag::OP_BUILD_COOP ||
                (commitment.op == kag::OP_PLANT && commitment.arg == kag::WHEAT);
        case 473:
            return commitment.op == kag::OP_PLANT && commitment.arg == kag::MELON;
        default: return false;
    }
}

int day17_target_index(const Commitment& commitment) {
    if (commitment.step / 24 != 17) return -1;
    static constexpr std::array<std::array<int16_t, 3>, 15> targets = {{
        {411, 3, 5}, {412, 4, 5}, {414, 2, 5}, {418, 4, 7}, {419, 0, 9},
        {421, 1, 5}, {421, 4, 8}, {424, 0, 5}, {424, 4, 9}, {427, 0, 6},
        {427, 3, 9}, {429, 0, 0}, {430, 0, 7}, {430, 3, 0}, {430, 3, 8},
    }};
    for (int index = 0; index < static_cast<int>(targets.size()); ++index)
        if (targets[index][1] == commitment.x && targets[index][2] == commitment.y)
            return index;
    std::abort();
}

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

Shadow make_shadow(const kag::agent::AgentObservation& observation) {
    Shadow shadow;
    shadow.day = observation.day;
    shadow.shed_total = observation.own.shed_total;
    for (int row = 0; row < kag::BOARD; ++row)
        for (int column = 0; column < kag::BOARD; ++column)
            shadow.tiles[row][column] = observation.self().tiles[row][column];
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        shadow.x[unit] = observation.self().pos_x[unit];
        shadow.y[unit] = observation.self().pos_y[unit];
        for (int item = 0; item < kag::N_ITEMS; ++item)
            shadow.inv[unit][item] = observation.own.inv[unit][item];
        shadow.inv_nkeys[unit] = observation.own.inv_nkeys[unit];
        for (int key = 0; key < shadow.inv_nkeys[unit]; ++key)
            shadow.inv_keys[unit][key] = observation.own.inv_keys[unit][key];
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        shadow.shed[item] = observation.own.shed[item];
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        shadow.seeds[crop] = observation.own.seeds[crop];
    return shadow;
}

void shadow_inv_add(Shadow& shadow, int unit, int item, int amount) {
    if (amount <= 0) return;
    if (!shadow.inv[unit][item])
        shadow.inv_keys[unit][shadow.inv_nkeys[unit]++] = static_cast<uint8_t>(item);
    shadow.inv[unit][item] = static_cast<kag::Count>(
        shadow.inv[unit][item] + amount);
}

void shadow_inv_take(Shadow& shadow, int unit, int item, int amount) {
    shadow.inv[unit][item] = static_cast<kag::Count>(
        shadow.inv[unit][item] - amount);
    if (shadow.inv[unit][item]) return;
    int key = 0;
    while (key < shadow.inv_nkeys[unit] && shadow.inv_keys[unit][key] != item) ++key;
    if (key == shadow.inv_nkeys[unit]) return;
    for (; key + 1 < shadow.inv_nkeys[unit]; ++key)
        shadow.inv_keys[unit][key] = shadow.inv_keys[unit][key + 1];
    --shadow.inv_nkeys[unit];
}

bool movement_legal(const Shadow& shadow, int unit, uint8_t op) {
    const int nx = shadow.x[unit] + (op == kag::OP_EAST) - (op == kag::OP_WEST);
    const int ny = shadow.y[unit] + (op == kag::OP_SOUTH) - (op == kag::OP_NORTH);
    return nx >= 0 && nx < kag::BOARD && ny >= 0 && ny < kag::BOARD;
}

bool executable(const Shadow& shadow, int unit, const kag::UnitAction& action) {
    const int x = shadow.x[unit];
    const int y = shadow.y[unit];
    const kag::Tile& tile = shadow.tiles[y][x];
    switch (action.op) {
        case kag::OP_PASS: return true;
        case kag::OP_NORTH: case kag::OP_SOUTH:
        case kag::OP_EAST: case kag::OP_WEST:
            return movement_legal(shadow, unit, action.op);
        case kag::OP_PICKUP:
            return shed_adjacent(x, y) && action.arg < kag::N_ITEMS &&
                action.n > 0 && shadow.shed[action.arg] > 0;
        case kag::OP_DROP:
            if (!shed_adjacent(x, y)) return false;
            for (int item = 0; item < kag::N_ITEMS; ++item)
                if (shadow.inv[unit][item] > 0) return true;
            return false;
        case kag::OP_PLACE: {
            if (action.arg >= kag::N_ITEMS || action.n <= 0) return false;
            if (kag::is_animal(action.arg)) {
                const kag::TileKind structure = kag::ANIMALS[action.arg - kag::GOOSE].structure ==
                    kag::ST_COOP ? kag::T_COOP : kag::T_PASTURE;
                if (tile.kind == structure && !tile.has_animal)
                    return shadow.inv[unit][action.arg] > 0;
            }
            return shed_adjacent(x, y) && shadow.inv[unit][action.arg] > 0 &&
                shadow.shed_total < 100;
        }
        case kag::OP_PLANT:
            return action.arg < kag::N_CROPS && tile.kind == kag::T_EMPTY &&
                shadow.seeds[action.arg] > 0;
        case kag::OP_WATER:
            return tile.kind == kag::T_PLANT && !tile.watered_today;
        case kag::OP_HARVEST:
            if (tile.yield_units <= 0) return false;
            if (tile.kind == kag::T_PLANT)
                return shadow.day - tile.planted_day >= kag::CROPS[tile.what].first_yield_day;
            return tile.has_animal;
        case kag::OP_FERTILIZE:
            return tile.kind == kag::T_PLANT && shadow.inv[unit][kag::FERTILIZER] > 0;
        case kag::OP_DIG:
            return tile.kind != kag::T_EMPTY && tile.kind != kag::T_LOCKED && !tile.has_animal;
        case kag::OP_BUILD_COOP: case kag::OP_BUILD_PASTURE:
            return tile.kind == kag::T_EMPTY;
        case kag::OP_FEED:
            return tile.has_animal && !tile.fed_today && shadow.inv[unit][kag::WHEAT] > 0;
        case kag::OP_COLLECT_FERTILIZER:
            return tile.has_animal && tile.fertilizer_available;
        case kag::OP_CARE:
            return tile.has_animal && !tile.cared_today;
        default: return false;
    }
}

bool already_satisfied(const Shadow& shadow, int unit, const kag::UnitAction& action) {
    const kag::Tile& tile = shadow.tiles[shadow.y[unit]][shadow.x[unit]];
    if (action.op == kag::OP_PLANT)
        return tile.kind == kag::T_PLANT && tile.what == action.arg;
    if (action.op == kag::OP_WATER)
        return tile.kind == kag::T_PLANT && tile.watered_today;
    if (action.op == kag::OP_BUILD_COOP)
        return tile.kind == kag::T_COOP;
    if (action.op == kag::OP_BUILD_PASTURE)
        return tile.kind == kag::T_PASTURE;
    if (action.op == kag::OP_FEED) return tile.has_animal && tile.fed_today;
    if (action.op == kag::OP_CARE) return tile.has_animal && tile.cared_today;
    if (action.op == kag::OP_COLLECT_FERTILIZER)
        return tile.has_animal && !tile.fertilizer_available;
    if (action.op == kag::OP_PLACE && kag::is_animal(action.arg))
        return tile.has_animal && tile.what == action.arg;
    return false;
}

void apply_shadow(Shadow& shadow, int unit, const kag::UnitAction& action) {
    int px = shadow.x[unit];
    int py = shadow.y[unit];
    if (action.op >= kag::OP_NORTH && action.op <= kag::OP_WEST) {
        px += (action.op == kag::OP_EAST) - (action.op == kag::OP_WEST);
        py += (action.op == kag::OP_SOUTH) - (action.op == kag::OP_NORTH);
        shadow.x[unit] = static_cast<int8_t>(px);
        shadow.y[unit] = static_cast<int8_t>(py);
        return;
    }
    kag::Tile& tile = shadow.tiles[py][px];
    switch (action.op) {
        case kag::OP_PICKUP: {
            const int amount = std::min<int>(action.n, shadow.shed[action.arg]);
            shadow.shed[action.arg] = static_cast<kag::Count>(
                shadow.shed[action.arg] - amount);
            shadow.shed_total -= amount;
            shadow_inv_add(shadow, unit, action.arg, amount);
            break;
        }
        case kag::OP_DROP: {
            const int keys = shadow.inv_nkeys[unit];
            for (int key = 0; key < keys; ++key) {
                const int item = shadow.inv_keys[unit][key];
                const int amount = std::min<int>(shadow.inv[unit][item],
                    std::max(0, 100 - shadow.shed_total));
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
                const kag::TileKind structure =
                    kag::ANIMALS[action.arg - kag::GOOSE].structure == kag::ST_COOP ?
                    kag::T_COOP : kag::T_PASTURE;
                if (tile.kind == structure && !tile.has_animal) {
                    shadow_inv_take(shadow, unit, action.arg, 1);
                    tile.what = action.arg;
                    tile.has_animal = true;
                    tile.planted_day = static_cast<int16_t>(shadow.day);
                    break;
                }
            }
            const int amount = std::min<int>({action.n, shadow.inv[unit][action.arg],
                static_cast<kag::Count>(std::max(0, 100 - shadow.shed_total))});
            shadow_inv_take(shadow, unit, action.arg, amount);
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
            const kag::CropDef& crop = kag::CROPS[tile.what];
            const int age = shadow.day - tile.planted_day;
            const int first_bonus_age = (crop.max_yield_day + 1) / 2;
            if (!crop.ongoing && age >= first_bonus_age && age <= crop.max_yield_day) {
                const int bonus = tile.fertilized_until_day >= shadow.day ? 2 : 1;
                tile.yield_units = static_cast<int8_t>(
                    std::min<int>(crop.max_yield, tile.yield_units + bonus));
            }
            break;
        }
        case kag::OP_HARVEST: {
            const int item = tile.kind == kag::T_PLANT ?
                static_cast<int>(tile.what) : static_cast<int>(
                    kag::ANIMALS[tile.what - kag::GOOSE].product);
            shadow_inv_add(shadow, unit, item, tile.yield_units);
            if (tile.kind == kag::T_PLANT && !kag::CROPS[tile.what].ongoing)
                tile = kag::Tile{};
            else tile.yield_units = 0;
            break;
        }
        case kag::OP_FERTILIZE:
            shadow_inv_take(shadow, unit, kag::FERTILIZER, 1);
            tile.fertilized_until_day = std::max<int16_t>(
                tile.fertilized_until_day, static_cast<int16_t>(shadow.day + 2));
            break;
        case kag::OP_DIG: tile = kag::Tile{}; break;
        case kag::OP_BUILD_COOP: tile = kag::Tile{}; tile.kind = kag::T_COOP; break;
        case kag::OP_BUILD_PASTURE: tile = kag::Tile{}; tile.kind = kag::T_PASTURE; break;
        case kag::OP_FEED:
            shadow_inv_take(shadow, unit, kag::WHEAT, 1);
            tile.fed_today = true;
            break;
        case kag::OP_COLLECT_FERTILIZER:
            tile.fertilizer_available = false;
            shadow_inv_add(shadow, unit, kag::FERTILIZER, 1);
            break;
        case kag::OP_CARE: tile.cared_today = true; break;
        default: break;
    }
}

}  // namespace

void CoreAgent::Queue::push_back(kag::UnitAction action) {
    if (size >= actions.size()) std::abort();
    actions[size++] = action;
}

void CoreAgent::Queue::push_front(kag::UnitAction action) {
    if (size >= actions.size()) std::abort();
    for (int index = size; index > 0; --index) actions[index] = actions[index - 1];
    actions[0] = action;
    ++size;
}

kag::UnitAction CoreAgent::Queue::pop_front() {
    if (!size) std::abort();
    const kag::UnitAction result = actions[0];
    for (int index = 1; index < size; ++index) actions[index - 1] = actions[index];
    --size;
    return result;
}

kag::agent::AgentInfo CoreAgent::info() { return {"fixed_weed_candidate"}; }

void CoreAgent::reset(const kag::agent::AgentInit& init) {
    player_ = init.player;
    last_day_ = -1;
    stats_ = {};
    cleanup_target_count_ = 0;
    cleanup_current_target_ = -1;
    cleanup_hire_requested_ = false;
    actual_sold_.fill(0);
    for (Queue& queue : queues_) queue.clear();
}

void CoreAgent::act(
    const kag::agent::AgentObservation& observation,
    const kag::agent::DecisionBudget&,
    kag::Action& action
) {
    if (observation.player != player_) std::abort();
    if (observation.day != last_day_) {
        for (Queue& queue : queues_) {
            stats_.day_boundary_drops += queue.size;
            queue.clear();
        }
        last_day_ = observation.day;
        cleanup_target_count_ = 0;
        cleanup_current_target_ = -1;
        cleanup_hire_requested_ = false;
        if (parameters_.cleanup_worker) {
            const NominalTable& table = nominal_table();
            const bool primary_cleanup_day =
                parameters_.cleanup_day_mask & (uint32_t{1} << observation.day);
            const bool recovery_cleanup_day =
                (parameters_.cleanup_recovery_day_mask &
                    (uint32_t{1} << observation.day)) && stats_.weed_digs > 0;
            const bool unconditional_cleanup_day =
                primary_cleanup_day || recovery_cleanup_day;
            const bool multi_cleanup_day =
                parameters_.cleanup_multi_day_mask & (uint32_t{1} << observation.day);
            for (int index = 0; index < table.commitment_count; ++index) {
                const Commitment& commitment = table.commitments[index];
                const int commitment_day = commitment.step / 24;
                const int kind = commitment.op == kag::OP_PLANT ? commitment.arg :
                    kag::N_CROPS + (commitment.op == kag::OP_BUILD_PASTURE);
                const int day17_index = day17_target_index(commitment);
                const bool current_day = commitment_day == observation.day;
                const bool future_day = commitment_day > observation.day &&
                    commitment_day - observation.day <=
                        parameters_.cleanup_lookahead_days;
                if ((!current_day && !future_day) ||
                    (current_day && day17_index >= 0 &&
                        !(parameters_.cleanup_day17_target_mask &
                            (uint16_t{1} << day17_index))) ||
                    (current_day && (parameters_.cleanup_value_day_mask &
                        (uint32_t{1} << observation.day)) &&
                        !positive_cleanup_counterfactual(commitment)) ||
                    (current_day && !unconditional_cleanup_day && !multi_cleanup_day &&
                        commitment.step % 24 < parameters_.cleanup_late_hour) ||
                    (current_day &&
                        !(parameters_.cleanup_kind_mask & (uint32_t{1} << kind))) ||
                    (future_day && !(parameters_.cleanup_lookahead_kind_mask &
                        (uint32_t{1} << kind))) ||
                    observation.self().tiles[commitment.y][commitment.x].kind != kag::T_WEED)
                    continue;
                const int16_t route_deadline = current_day ? commitment.step :
                    static_cast<int16_t>((observation.day + 1) * 24);
                int existing = -1;
                for (int target = 0; target < cleanup_target_count_; ++target)
                    if (cleanup_targets_[target].x == commitment.x &&
                        cleanup_targets_[target].y == commitment.y)
                        existing = target;
                if (existing >= 0) {
                    cleanup_targets_[existing].deadline = std::min(
                        cleanup_targets_[existing].deadline, route_deadline);
                    cleanup_targets_[existing].weight = std::max(
                        cleanup_targets_[existing].weight, cleanup_weight(commitment));
                    cleanup_targets_[existing].hire_eligible |= current_day;
                } else {
                    if (cleanup_target_count_ >= static_cast<int>(cleanup_targets_.size()))
                        std::abort();
                    cleanup_targets_[cleanup_target_count_++] = {
                        route_deadline, commitment.x, commitment.y,
                        cleanup_weight(commitment), true, current_day};
                }
            }
            if (!unconditional_cleanup_day && multi_cleanup_day) {
                int current_targets = 0;
                for (int target = 0; target < cleanup_target_count_; ++target)
                    current_targets += cleanup_targets_[target].hire_eligible;
                if (current_targets < parameters_.cleanup_multi_min_targets) {
                    int write = 0;
                    for (int target = 0; target < cleanup_target_count_; ++target)
                        if (!cleanup_targets_[target].hire_eligible)
                            cleanup_targets_[write++] = cleanup_targets_[target];
                    cleanup_target_count_ = write;
                }
            }
        }
    }

    action.clear();
    action.n_units = 1 + observation.own_hand_count();
    for (int unit = 0; unit < action.n_units; ++unit) action.units[unit] = {};
    if (observation.step < 0 || observation.step >= TAPE_STEPS) {
        action.finalize();
        return;
    }

    Shadow shadow = make_shadow(observation);
    int8_t repair_crop[kag::BOARD][kag::BOARD];
    bool urgent_water[kag::BOARD][kag::BOARD]{};
    for (auto& row : repair_crop) std::fill(std::begin(row), std::end(row), int8_t{-1});
    for (int unit = 0; unit < action.n_units; ++unit) {
        if (parameters_.cleanup_worker && unit >= nominal_units(observation.step)) {
            int target_index = -1;
            if (cleanup_current_target_ >= 0) {
                CleanupTarget& current = cleanup_targets_[cleanup_current_target_];
                const int distance = std::abs(shadow.x[unit] - current.x) +
                    std::abs(shadow.y[unit] - current.y);
                current.active = current.active &&
                    shadow.tiles[current.y][current.x].kind == kag::T_WEED;
                if (current.active && observation.step + distance < current.deadline)
                    target_index = cleanup_current_target_;
                else
                    cleanup_current_target_ = -1;
            }
            std::array<int8_t, kag::BOARD * kag::BOARD> active{};
            int active_count = 0;
            for (int target = 0; target < cleanup_target_count_; ++target) {
                CleanupTarget& candidate = cleanup_targets_[target];
                if (!candidate.active) continue;
                if (shadow.tiles[candidate.y][candidate.x].kind != kag::T_WEED) {
                    candidate.active = false;
                    continue;
                }
                const int distance = std::abs(shadow.x[unit] - candidate.x) +
                    std::abs(shadow.y[unit] - candidate.y);
                if (observation.step + distance >= candidate.deadline) continue;
                active[active_count++] = static_cast<int8_t>(target);
                if (cleanup_current_target_ < 0 &&
                    (target_index < 0 || candidate.deadline <
                        cleanup_targets_[target_index].deadline))
                    target_index = target;
            }
            if (cleanup_current_target_ < 0 && active_count > 1 &&
                active_count <= EXACT_CLEANUP_TARGETS) {
                const int states = 1 << active_count;
                const int cells = states * EXACT_CLEANUP_TARGETS;
                std::fill_n(cleanup_finish_.begin(), cells,
                    std::numeric_limits<int16_t>::max());
                std::fill_n(cleanup_first_.begin(), cells, int8_t{-1});
                for (int index = 0; index < active_count; ++index) {
                    const CleanupTarget& target = cleanup_targets_[active[index]];
                    const int completion = observation.step +
                        std::abs(shadow.x[unit] - target.x) +
                        std::abs(shadow.y[unit] - target.y);
                    const int cell = (1 << index) * EXACT_CLEANUP_TARGETS + index;
                    cleanup_finish_[cell] = static_cast<int16_t>(completion);
                    cleanup_first_[cell] = static_cast<int8_t>(index);
                }
                int best_count = 0;
                int best_weight = 0;
                int best_deadline_sum = std::numeric_limits<int>::max();
                int best_finish = std::numeric_limits<int>::max();
                for (int mask = 1; mask < states; ++mask) {
                    int mask_weight = 0;
                    int deadline_sum = 0;
                    for (int index = 0; index < active_count; ++index)
                        if (mask & (1 << index)) {
                            mask_weight += cleanup_targets_[active[index]].weight;
                            deadline_sum += cleanup_targets_[active[index]].deadline;
                        }
                    const int count = std::popcount(static_cast<unsigned>(mask));
                    for (int last = 0; last < active_count; ++last) {
                        const int cell = mask * EXACT_CLEANUP_TARGETS + last;
                        const int completion = cleanup_finish_[cell];
                        if (completion == std::numeric_limits<int16_t>::max()) continue;
                        if (count > best_count ||
                            (count == best_count && deadline_sum < best_deadline_sum) ||
                            (count == best_count && deadline_sum == best_deadline_sum &&
                                mask_weight > best_weight) ||
                            (count == best_count && deadline_sum == best_deadline_sum &&
                                mask_weight == best_weight && completion < best_finish)) {
                            target_index = active[cleanup_first_[cell]];
                            best_count = count;
                            best_weight = mask_weight;
                            best_deadline_sum = deadline_sum;
                            best_finish = completion;
                        }
                        for (int next = 0; next < active_count; ++next) {
                            if (mask & (1 << next)) continue;
                            const CleanupTarget& from = cleanup_targets_[active[last]];
                            const CleanupTarget& to = cleanup_targets_[active[next]];
                            const int next_completion = completion + 1 +
                                std::abs(from.x - to.x) + std::abs(from.y - to.y);
                            if (next_completion >= to.deadline) continue;
                            const int next_cell =
                                (mask | (1 << next)) * EXACT_CLEANUP_TARGETS + next;
                            if (next_completion < cleanup_finish_[next_cell]) {
                                cleanup_finish_[next_cell] = static_cast<int16_t>(next_completion);
                                cleanup_first_[next_cell] = cleanup_first_[cell];
                            }
                        }
                    }
                }
            }
            if (target_index >= 0) {
                cleanup_current_target_ = target_index;
                CleanupTarget& target = cleanup_targets_[target_index];
                const int x = shadow.x[unit];
                const int y = shadow.y[unit];
                if (x == target.x && y == target.y) {
                    action.units[unit] = {kag::OP_DIG, 0, 1};
                    target.active = false;
                    cleanup_current_target_ = -1;
                    ++stats_.cleanup_digs;
                } else if (x < target.x) {
                    action.units[unit] = {kag::OP_EAST, 0, 1};
                } else if (x > target.x) {
                    action.units[unit] = {kag::OP_WEST, 0, 1};
                } else if (y < target.y) {
                    action.units[unit] = {kag::OP_SOUTH, 0, 1};
                } else {
                    action.units[unit] = {kag::OP_NORTH, 0, 1};
                }
                apply_shadow(shadow, unit, action.units[unit]);
                continue;
            }
        }
        Queue& queue = queues_[unit];
        const kag::UnitAction nominal = nominal_unit(observation.step, unit);
        kag::UnitAction selected = nominal;
        bool from_queue = queue.size > 0;
        if (from_queue) {
            selected = queue.pop_front();
            ++stats_.queued_actions;
            if (nominal.op != kag::OP_PASS) queue.push_back(nominal);
        }

        for (int attempt = 0; attempt < QUEUE_CAPACITY; ++attempt) {
            const int x = shadow.x[unit];
            const int y = shadow.y[unit];
            const kag::Tile& tile = shadow.tiles[y][x];

            if (tile.kind == kag::T_EMPTY && repair_crop[y][x] >= 0 &&
                (selected.op == kag::OP_WATER || selected.op == kag::OP_PASS) &&
                shadow.seeds[repair_crop[y][x]] > 0) {
                if (selected.op == kag::OP_WATER)
                    queue.push_front({kag::OP_WATER, 0, 1});
                action.units[unit] = {
                    kag::OP_PLANT, static_cast<uint8_t>(repair_crop[y][x]), 1};
                repair_crop[y][x] = -1;
                urgent_water[y][x] = true;
                apply_shadow(shadow, unit, action.units[unit]);
                stats_.maximum_queue = std::max<int>(stats_.maximum_queue, queue.size);
                break;
            }

            if (selected.op == kag::OP_PASS && urgent_water[y][x] &&
                tile.kind == kag::T_PLANT && !tile.watered_today) {
                action.units[unit] = {kag::OP_WATER, 0, 1};
                urgent_water[y][x] = false;
                apply_shadow(shadow, unit, action.units[unit]);
                break;
            }

            if ((selected.op == kag::OP_PLANT || selected.op == kag::OP_BUILD_COOP ||
                 selected.op == kag::OP_BUILD_PASTURE) && tile.kind == kag::T_WEED) {
                if (selected.op == kag::OP_PLANT &&
                    parameters_.queue_water_after_repaired_plant)
                    queue.push_front({kag::OP_WATER, 0, 1});
                queue.push_front(selected);
                ++stats_.weed_digs;
                stats_.repaired_plants += selected.op == kag::OP_PLANT;
                stats_.repaired_builds += selected.op != kag::OP_PLANT;
                stats_.maximum_queue = std::max<int>(stats_.maximum_queue, queue.size);
                action.units[unit] = {kag::OP_DIG, 0, 1};
                if (selected.op == kag::OP_PLANT)
                    repair_crop[y][x] = static_cast<int8_t>(selected.arg);
                apply_shadow(shadow, unit, action.units[unit]);
                break;
            }

            if (selected.op == kag::OP_PLACE && kag::is_animal(selected.arg) &&
                parameters_.repair_animal_place && shadow.inv[unit][selected.arg] > 0 &&
                (tile.kind == kag::T_WEED || tile.kind == kag::T_EMPTY)) {
                const uint8_t build = kag::ANIMALS[selected.arg - kag::GOOSE].structure ==
                    kag::ST_COOP ? kag::OP_BUILD_COOP : kag::OP_BUILD_PASTURE;
                queue.push_front(selected);
                if (tile.kind == kag::T_WEED) {
                    queue.push_front({build, 0, 1});
                    action.units[unit] = {kag::OP_DIG, 0, 1};
                    ++stats_.weed_digs;
                } else {
                    action.units[unit] = {build, 0, 1};
                }
                ++stats_.repaired_places;
                stats_.maximum_queue = std::max<int>(stats_.maximum_queue, queue.size);
                apply_shadow(shadow, unit, action.units[unit]);
                break;
            }

            if (executable(shadow, unit, selected)) {
                action.units[unit] = selected;
                apply_shadow(shadow, unit, selected);
                break;
            }

            if (!from_queue ||
                (!already_satisfied(shadow, unit, selected) && !queue.size)) {
                stats_.stale_actions += selected.op != kag::OP_PASS;
                action.units[unit] = {};
                break;
            }
            if (!queue.size) {
                action.units[unit] = {};
                break;
            }
            selected = queue.pop_front();
            ++stats_.queued_actions;
            from_queue = true;
        }
    }

    nominal_orders(observation.step, action);
    if (parameters_.clamp_market_orders) {
        double money = observation.self().money;
        std::array<int, kag::N_ITEMS> shed{};
        for (int item = 0; item < kag::N_ITEMS; ++item) shed[item] = shadow.shed[item];
        int shed_total = shadow.shed_total;
        int hires = observation.self().hires_today;
        int units = observation.self().n_units;
        int quadrants = observation.self().n_quadrants;
        std::array<int, kag::N_PRODUCTS> market{};
        for (int item = 0; item < kag::N_PRODUCTS; ++item)
            market[item] = observation.market.inventory[item];
        int write = 0;
        std::array<int, kag::N_PRODUCTS> scheduled_sells{};
        const auto& final_nominal_sold =
            nominal_table().sold_through_step[TAPE_STEPS - 1];
        auto fund_to = [&](double required_money, uint8_t purchase_op,
                           uint8_t purchase_item, int purchase_n) {
            if (!parameters_.liquidity_sales || money >= required_money || write >= 9)
                return;
            const double cash_before = money;
            int best_item = -1;
            int best_price = -1;
            double best_estimated_delta = -1e13;
            if (parameters_.liquidity_opportunity_selector) {
                for (int candidate_pass = 0; candidate_pass < 2 && best_item < 0;
                     ++candidate_pass) {
                    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                        const bool production_input = item == kag::WHEAT ||
                            item == kag::FERTILIZER;
                        if (candidate_pass == 0 && production_input) continue;
                        const int sale_capacity = std::min(shed[item],
                            final_nominal_sold[item] - actual_sold_[item] -
                                scheduled_sells[item]);
                        int candidate_quantity = 0;
                        int candidate_inventory = market[item];
                        double candidate_money = money;
                        double candidate_proceeds = 0;
                        while (candidate_quantity < sale_capacity &&
                               candidate_money < required_money) {
                            const int price = kag::market_price(item,
                                candidate_inventory);
                            candidate_money += price;
                            candidate_proceeds += price;
                            candidate_inventory += price > 1;
                            ++candidate_quantity;
                        }
                        if (candidate_money < required_money) continue;
                        const double estimated_delta = shifted_sale_revenue_delta(
                            observation.step, item, candidate_quantity, market[item],
                            candidate_proceeds);
                        if (estimated_delta > best_estimated_delta ||
                            (estimated_delta == best_estimated_delta &&
                             item < best_item)) {
                            best_item = item;
                            best_price = kag::market_price(item, market[item]);
                            best_estimated_delta = estimated_delta;
                        }
                    }
                }
            }
            if (!parameters_.liquidity_opportunity_selector &&
                parameters_.liquidity_preferred_item >= 0 &&
                parameters_.liquidity_preferred_item < kag::N_PRODUCTS) {
                const int item = parameters_.liquidity_preferred_item;
                const int sale_capacity = std::min(shed[item],
                    final_nominal_sold[item] - actual_sold_[item] -
                        scheduled_sells[item]);
                if (sale_capacity > 0) {
                    best_item = item;
                    best_price = kag::market_price(item, market[item]);
                }
            }
            if (best_item < 0 && !parameters_.liquidity_opportunity_selector) {
                for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                    const int sale_capacity = std::min(shed[item],
                        final_nominal_sold[item] - actual_sold_[item] -
                            scheduled_sells[item]);
                    if (sale_capacity <= 0) continue;
                    const int price = kag::market_price(item, market[item]);
                    if (price > best_price) {
                        best_item = item;
                        best_price = price;
                    }
                }
            }
            if (best_item < 0) return;
            const int sale_capacity = std::min(shed[best_item],
                final_nominal_sold[best_item] - actual_sold_[best_item] -
                    scheduled_sells[best_item]);
            int quantity = 0;
            while (quantity < sale_capacity && money < required_money) {
                money += kag::market_price(best_item, market[best_item]);
                --shed[best_item];
                --shed_total;
                if (kag::market_price(best_item, market[best_item]) > 1)
                    ++market[best_item];
                ++quantity;
            }
            if (quantity <= 0) return;
            action.orders[write++] = {
                kag::M_SELL, static_cast<uint8_t>(best_item), quantity};
            scheduled_sells[best_item] += quantity;
            stats_.liquidity_sold += quantity;
            stats_.liquidity_sold_by_day[observation.day] += quantity;
            stats_.liquidity_sold_by_item[best_item] += quantity;
            if (stats_.funding_event_count < stats_.funding_events.size()) {
                auto& event = stats_.funding_events[stats_.funding_event_count++];
                event.step = static_cast<int16_t>(observation.step);
                event.purchase_op = purchase_op;
                event.purchase_item = purchase_item;
                event.purchase_n = static_cast<int16_t>(purchase_n);
                event.sale_item = static_cast<int8_t>(best_item);
                event.sale_n = static_cast<int16_t>(quantity);
                event.cash_before = cash_before;
                event.required = required_money;
                event.proceeds = money - cash_before;
                event.estimated_revenue_delta = best_estimated_delta;
            }
        };
        int nominal_sell_underfill = 0;
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order order = action.orders[index];
            const int requested = order.n;
            int accepted = 0;
            if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS) {
                const int limit = std::min<int>(order.n, shed[order.item]);
                for (; accepted < limit; ++accepted) {
                    const int price = kag::market_price(order.item, market[order.item]);
                    money += price;
                    --shed[order.item];
                    --shed_total;
                    if (price > 1) ++market[order.item];
                }
            } else if (order.op == kag::M_BUY_SEED && order.item < kag::N_CROPS) {
                const int price = kag::CROPS[order.item].seed;
                fund_to(static_cast<double>(order.n * price), order.op,
                    order.item, order.n);
                accepted = std::min<int>(
                    order.n, static_cast<int>(money / price));
                money -= accepted * price;
            } else if (order.op == kag::M_BUY_PRODUCT &&
                       (order.item == kag::WHEAT || order.item == kag::FERTILIZER)) {
                while (accepted < order.n && shed_total < 100) {
                    const int price = kag::market_price(order.item, market[order.item] - 1);
                    if (money < price) break;
                    money -= price;
                    --market[order.item];
                    ++shed[order.item];
                    ++shed_total;
                    ++accepted;
                }
            } else if (order.op == kag::M_BUY_ANIMAL && kag::is_animal(order.item)) {
                const int price = kag::ANIMALS[order.item - kag::GOOSE].cost;
                fund_to(static_cast<double>(order.n * price), order.op,
                    order.item, order.n);
                while (accepted < order.n && shed_total < 100 && money >= price) {
                    money -= price;
                    ++shed[order.item];
                    ++shed_total;
                    ++accepted;
                }
            } else if (order.op == kag::M_HIRE) {
                const int price = kag::fib(hires);
                fund_to(price, order.op, order.item, order.n);
                if (money >= price && units < kag::MAX_UNITS) {
                    money -= price;
                    ++hires;
                    ++units;
                    accepted = 1;
                }
            } else if (order.op == kag::M_BUY_LAND && quadrants < 4) {
                const int price = kag::LAND_PRICES[quadrants - 1];
                fund_to(price, order.op, order.item, order.n);
                if (money >= price) {
                    money -= price;
                    ++quadrants;
                    accepted = 1;
                }
            }
            if (accepted > 0) {
                order.n = accepted;
                action.orders[write++] = order;
                if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
                    scheduled_sells[order.item] += accepted;
            }
            if (order.op == kag::M_SELL)
                nominal_sell_underfill += requested - accepted;
        }
        if (parameters_.underfilled_sale_substitution && observation.hour == 23 &&
            nominal_sell_underfill > 0 && write < 10) {
            std::array<int, kag::N_PRODUCTS> owned_before{};
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                owned_before[item] = observation.own.shed[item];
            for (int unit = 0; unit < observation.self().n_units; ++unit)
                for (int item = 0; item < kag::N_PRODUCTS; ++item)
                    owned_before[item] += observation.own.inv[unit][item];
            while (nominal_sell_underfill > 0 && write < 10) {
                int best_item = -1;
                int best_price = -1;
                int best_surplus = 0;
                for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                    const int nominal_owned = nominal_table().owned_before_step[
                        observation.step][item];
                    const int surplus = std::min<int>(shed[item],
                        owned_before[item] - nominal_owned);
                    if (surplus <= 0) continue;
                    const int price = kag::market_price(item, market[item]);
                    if (price > best_price) {
                        best_item = item;
                        best_price = price;
                        best_surplus = surplus;
                    }
                }
                if (best_item < 0) break;
                const int quantity = std::min(nominal_sell_underfill, best_surplus);
                action.orders[write++] = {
                    kag::M_SELL, static_cast<uint8_t>(best_item), quantity};
                for (int unit = 0; unit < quantity; ++unit) {
                    const int price = kag::market_price(best_item, market[best_item]);
                    money += price;
                    --shed[best_item];
                    --shed_total;
                    market[best_item] += price > 1;
                }
                scheduled_sells[best_item] += quantity;
                stats_.substituted_sales += quantity;
                nominal_sell_underfill -= quantity;
            }
        }
        const int last_nominal_hire = observation.day < 30 ?
            nominal_table().last_hire_step[observation.day] : -1;
        const int safe_cleanup_hire_step = last_nominal_hire >= 0 ?
            last_nominal_hire : observation.day * 24;
        const int accepted_hires = units - observation.self().n_units;
        const std::array<int, 2> spawn = next_spawn(observation, accepted_hires);
        bool has_reachable_cleanup = false;
        for (int target = 0; target < cleanup_target_count_; ++target) {
            CleanupTarget& candidate = cleanup_targets_[target];
            if (!candidate.active || !candidate.hire_eligible) continue;
            if (shadow.tiles[candidate.y][candidate.x].kind != kag::T_WEED) {
                candidate.active = false;
                continue;
            }
            const int distance = std::abs(spawn[0] - candidate.x) +
                std::abs(spawn[1] - candidate.y);
            has_reachable_cleanup |= observation.step + 1 + distance < candidate.deadline;
        }
        if (parameters_.cleanup_worker && has_reachable_cleanup &&
            !cleanup_hire_requested_ && observation.step >= safe_cleanup_hire_step && write < 10) {
            const int price = kag::fib(hires);
            if (money >= price && units < kag::MAX_UNITS) {
                action.orders[write++] = {kag::M_HIRE, 0, 1};
                cleanup_hire_requested_ = true;
                ++stats_.cleanup_hires;
            }
        }
        if (parameters_.catch_up_sales) {
            const auto& nominal_sold =
                nominal_table().sold_through_step[observation.step];
            for (int item = 0; item < kag::N_PRODUCTS && write < 10; ++item) {
                const int deficit = nominal_sold[item] - actual_sold_[item] -
                    scheduled_sells[item];
                const int quantity = std::min(deficit, shed[item]);
                if (quantity <= 0) continue;
                action.orders[write++] = {
                    kag::M_SELL, static_cast<uint8_t>(item), quantity};
                shed[item] -= quantity;
                shed_total -= quantity;
                scheduled_sells[item] += quantity;
                stats_.catch_up_sold += quantity;
            }
        }
        for (int item = 0; item < kag::N_PRODUCTS; ++item)
            actual_sold_[item] += scheduled_sells[item];
        action.n_orders = write;
    }
    action.finalize();
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::pizza_125556_robust::detail
