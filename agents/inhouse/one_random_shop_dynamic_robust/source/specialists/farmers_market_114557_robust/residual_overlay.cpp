#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/farmers_market_114557_robust/residual_overlay.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::farmers_market_114557_robust::detail::residual {
namespace tape {
#ifndef FIXED_WEED_RESIDUAL_TAPE_INCLUDE
#define FIXED_WEED_RESIDUAL_TAPE_INCLUDE \
    "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/farmers_market_114557_robust/tape.inc"
#endif
#include FIXED_WEED_RESIDUAL_TAPE_INCLUDE
}
namespace {

struct Day17Target {
    int8_t x = 0;
    int8_t y = 0;
};

constexpr std::array<Day17Target, 15> DAY17_TARGETS = {{
    {3, 5}, {4, 5}, {2, 5}, {4, 7}, {0, 9},
    {1, 5}, {4, 8}, {0, 5}, {4, 9}, {0, 6},
    {3, 9}, {0, 0}, {0, 7}, {3, 0}, {3, 8},
}};

int nominal_hires(int step) {
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    cursor += 3 * units;
    int result = 0;
    for (int order = 0; order < orders; ++order) {
        result += tape::TAPE_DATA[cursor] == kag::M_HIRE;
        cursor += 3;
    }
    return result;
}

bool obviously_fails(const kag::agent::AgentObservation& observation,
                     int unit, const kag::UnitAction& action,
                     const std::array<int, kag::N_CROPS>& plant_demand,
                     const kag::Tile& tile) {
    if (action.op == kag::OP_PASS) return false;
    const int x = observation.self().pos_x[unit];
    const int y = observation.self().pos_y[unit];
    if (action.op >= kag::OP_NORTH && action.op <= kag::OP_WEST) {
        const int nx = x + (action.op == kag::OP_EAST) -
            (action.op == kag::OP_WEST);
        const int ny = y + (action.op == kag::OP_SOUTH) -
            (action.op == kag::OP_NORTH);
        return nx < 0 || nx >= kag::BOARD || ny < 0 || ny >= kag::BOARD;
    }
    if (action.op == kag::OP_DROP) {
        if (!kag::is_shed_adjacent(x, y, kag::BOARD)) return true;
        for (int item = 0; item < kag::N_ITEMS; ++item)
            if (observation.own.inv[unit][item] > 0) return false;
        return true;
    }
    if (action.op == kag::OP_PICKUP)
        return !kag::is_shed_adjacent(x, y, kag::BOARD) || action.n <= 0 ||
            action.arg >= kag::N_ITEMS || observation.own.shed[action.arg] <= 0;
    if (action.op == kag::OP_PLACE) {
        if (action.arg >= kag::N_ITEMS || action.n <= 0 ||
            observation.own.inv[unit][action.arg] <= 0)
            return true;
        if (kag::is_animal(action.arg)) {
            const kag::TileKind required =
                kag::ANIMALS[action.arg - kag::GOOSE].structure == kag::ST_COOP ?
                    kag::T_COOP : kag::T_PASTURE;
            return tile.kind != required || tile.has_animal;
        }
        return !kag::is_shed_adjacent(x, y, kag::BOARD) ||
            observation.own.shed_total >= 100;
    }
    if (tile.kind == kag::T_LOCKED) return true;
    switch (action.op) {
        case kag::OP_PLANT:
            return action.arg >= kag::N_CROPS || tile.kind != kag::T_EMPTY ||
                observation.own.seeds[action.arg] <= 0 ||
                plant_demand[action.arg] > observation.own.seeds[action.arg];
        case kag::OP_WATER:
            return tile.kind != kag::T_PLANT || tile.watered_today;
        case kag::OP_HARVEST:
            if (tile.kind == kag::T_EMPTY || tile.kind == kag::T_WEED ||
                tile.yield_units <= 0)
                return true;
            return tile.kind == kag::T_PLANT &&
                observation.day - tile.planted_day <
                    kag::CROPS[tile.what].first_yield_day;
        case kag::OP_FERTILIZE:
            return tile.kind != kag::T_PLANT ||
                observation.own.inv[unit][kag::FERTILIZER] <= 0;
        case kag::OP_DIG:
            return tile.kind == kag::T_EMPTY || tile.has_animal;
        case kag::OP_BUILD_COOP:
        case kag::OP_BUILD_PASTURE:
            return tile.kind != kag::T_EMPTY;
        case kag::OP_FEED:
            return !tile.has_animal || tile.fed_today ||
                observation.own.inv[unit][kag::WHEAT] <= 0;
        case kag::OP_COLLECT_FERTILIZER:
            return !tile.has_animal || !tile.fertilizer_available;
        case kag::OP_CARE:
            return !tile.has_animal || tile.cared_today;
        default:
            return true;
    }
}

bool apply_tile_effect(kag::Tile& tile, const kag::UnitAction& action,
                       int day) {
    switch (action.op) {
        case kag::OP_PLANT: {
            kag::Tile planted{};
            planted.kind = kag::T_PLANT;
            planted.what = action.arg;
            planted.planted_day = static_cast<int16_t>(day);
            planted.consecutive_dry = 1;
            planted.yield_units = kag::CROPS[action.arg].ongoing ? 0 : 1;
            tile = planted;
            return true;
        }
        case kag::OP_WATER:
            tile.watered_today = true;
            return true;
        case kag::OP_HARVEST:
            if (tile.kind == kag::T_PLANT &&
                !kag::CROPS[tile.what].ongoing)
                tile = kag::Tile{};
            else
                tile.yield_units = 0;
            return true;
        case kag::OP_FERTILIZE:
            tile.fertilized_until_day = static_cast<int16_t>(day + 2);
            return true;
        case kag::OP_DIG:
            tile = kag::Tile{};
            return true;
        case kag::OP_BUILD_COOP:
            tile = kag::Tile{};
            tile.kind = kag::T_COOP;
            return true;
        case kag::OP_BUILD_PASTURE:
            tile = kag::Tile{};
            tile.kind = kag::T_PASTURE;
            return true;
        case kag::OP_PLACE:
            if (!kag::is_animal(action.arg)) return false;
            tile.what = action.arg;
            tile.has_animal = true;
            tile.planted_day = static_cast<int16_t>(day);
            return true;
        case kag::OP_FEED:
            tile.fed_today = true;
            return true;
        case kag::OP_COLLECT_FERTILIZER:
            tile.fertilizer_available = false;
            return true;
        case kag::OP_CARE:
            tile.cared_today = true;
            return true;
        default:
            return false;
    }
}

void suppress_obvious_failures(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    struct TileChange {
        int8_t x = 0;
        int8_t y = 0;
        kag::Tile tile{};
    };
    std::array<int, kag::N_CROPS> plant_demand{};
    std::array<TileChange, kag::MAX_UNITS> changes{};
    int change_count = 0;
    for (int unit = 0; unit < action.n_units; ++unit)
        if (action.units[unit].op == kag::OP_PLANT &&
            action.units[unit].arg < kag::N_CROPS)
            ++plant_demand[action.units[unit].arg];
    for (int unit = 0; unit < action.n_units; ++unit) {
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        kag::Tile tile = observation.self().tiles[y][x];
        int change_index = -1;
        for (int index = 0; index < change_count; ++index)
            if (changes[index].x == x && changes[index].y == y) {
                change_index = index;
                tile = changes[index].tile;
            }
        if (obviously_fails(
                observation, unit, action.units[unit], plant_demand, tile)) {
#if defined(ONE_SHOP_TRACE_OBVIOUS_FAILURE_GUARD) && \
    ONE_SHOP_TRACE_OBVIOUS_FAILURE_GUARD
            std::fprintf(stderr,
                "guard step=%d unit=%d pos=%d,%d op=%d arg=%d n=%d tile=%d what=%d yield=%d\n",
                observation.step, unit, observation.self().pos_x[unit],
                observation.self().pos_y[unit], action.units[unit].op,
                action.units[unit].arg, action.units[unit].n,
                observation.self().tiles[observation.self().pos_y[unit]]
                    [observation.self().pos_x[unit]].kind,
                observation.self().tiles[observation.self().pos_y[unit]]
                    [observation.self().pos_x[unit]].what,
                observation.self().tiles[observation.self().pos_y[unit]]
                    [observation.self().pos_x[unit]].yield_units);
#endif
            action.units[unit] = {};
            continue;
        }
        if (!apply_tile_effect(tile, action.units[unit], observation.day))
            continue;
        if (change_index < 0) {
            if (change_count >= static_cast<int>(changes.size())) std::abort();
            change_index = change_count++;
            changes[change_index].x = static_cast<int8_t>(x);
            changes[change_index].y = static_cast<int8_t>(y);
        }
        changes[change_index].tile = tile;
    }
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    stats_ = {};
}

void Overlay::modify(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    if (observation.player != player_) std::abort();
    if (!(parameters_.suppress_cleanup_hire_day_mask &
          (uint32_t{1} << observation.day))) {
        suppress_obvious_failures(observation, action);
        action.finalize();
        return;
    }
    int action_hires = 0;
    for (int order = 0; order < action.n_orders; ++order)
        action_hires += action.orders[order].op == kag::M_HIRE;
    int excess = action_hires - nominal_hires(observation.step);
    if (excess > 0 && observation.day == 17) {
        const uint16_t target_mask = parameters_.day17_target_mask;
        if (!target_mask) {
            suppress_obvious_failures(observation, action);
            action.finalize();
            return;
        }
        int weed_count = 0;
        int target_index = -1;
        for (int index = 0; index < static_cast<int>(DAY17_TARGETS.size());
             ++index) {
            const Day17Target& target = DAY17_TARGETS[index];
            if (observation.self().tiles[target.y][target.x].kind != kag::T_WEED)
                continue;
            ++weed_count;
            target_index = index;
        }
        const bool require_single = parameters_.require_single_day17_target;
        if (require_single && weed_count != 1) {
            ++stats_.rejected_multiple_targets;
            suppress_obvious_failures(observation, action);
            action.finalize();
            return;
        }
        const bool targets_allowed = target_index >= 0 &&
            (target_mask & (uint16_t{1} << target_index));
        if (!targets_allowed) {
            ++stats_.rejected_target_mask;
            suppress_obvious_failures(observation, action);
            action.finalize();
            return;
        }
    }
    while (excess-- > 0) {
        int remove = action.n_orders - 1;
        while (remove >= 0 && action.orders[remove].op != kag::M_HIRE)
            --remove;
        if (remove < 0) std::abort();
        for (int order = remove + 1; order < action.n_orders; ++order)
            action.orders[order - 1] = action.orders[order];
        --action.n_orders;
        ++stats_.suppressed_cleanup_hires;
        for (int y = 0; y < kag::BOARD; ++y)
            for (int x = 0; x < kag::BOARD; ++x) {
                if (observation.self().tiles[y][x].kind != kag::T_WEED)
                    continue;
                const int tile = y * kag::BOARD + x;
                stats_.suppression_weed_mask[tile >> 6] |=
                    uint64_t{1} << (tile & 63);
                ++stats_.suppression_weed_count;
            }
    }
    suppress_obvious_failures(observation, action);
    action.finalize();
}

}  // namespace kag::agents::farmers_market_114557_robust::detail::residual
