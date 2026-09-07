#include "residual_route_overlay.hpp"

#include <array>
#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::brunch_114643_robust::detail::residual_route {
namespace tape {
#ifndef FIXED_WEED_RESIDUAL_TAPE_INCLUDE
#define FIXED_WEED_RESIDUAL_TAPE_INCLUDE \
    "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/brunch_114643_robust/tape.inc"
#endif
#include FIXED_WEED_RESIDUAL_TAPE_INCLUDE
}
namespace {

struct Target { int8_t x = 0; int8_t y = 0; };
constexpr std::array<Target, 15> TARGETS = {{
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

int hires(const kag::Action& action) {
    int result = 0;
    for (int order = 0; order < action.n_orders; ++order)
        result += action.orders[order].op == kag::M_HIRE;
    return result;
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    suppressed_target_ = -1;
    target1_check_ = false;
    target6_route_ = false;
    stats_ = {};
}

void Overlay::verify_route(const kag::agent::AgentObservation& observation) {
    if (target1_check_ && observation.step == 410) {
        if (observation.self().tiles[5][4].kind == kag::T_WEED)
            ++stats_.route_failures;
        else {
            ++stats_.completed_routes;
            target1_check_ = false;
        }
    }
    if (!target6_route_) return;
    const int step = observation.step;
    bool valid = true;
    if (step >= 414 && step <= 420) {
        constexpr std::array<int8_t, 7> x = {4, 4, 4, 3, 2, 1, 0};
        constexpr std::array<int8_t, 7> y = {8, 8, 9, 9, 9, 9, 9};
        valid = observation.self().n_units > 11 &&
            observation.self().pos_x[11] == x[static_cast<size_t>(step - 414)] &&
            observation.self().pos_y[11] == y[static_cast<size_t>(step - 414)];
        if (step == 415)
            valid &= observation.self().tiles[8][4].kind == kag::T_EMPTY;
    } else if (step == 421) {
        const auto& tile = observation.self().tiles[9][0];
        valid = tile.kind == kag::T_PLANT && tile.what == kag::MELON;
    } else if (step == 422) {
        const auto& tile = observation.self().tiles[9][0];
        valid = tile.kind == kag::T_PLANT && tile.what == kag::MELON &&
            tile.watered_today;
        if (valid) ++stats_.completed_routes;
        target6_route_ = false;
    }
    if (!valid) {
        ++stats_.route_failures;
        target6_route_ = false;
    }
}

void Overlay::apply_target6_route(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!target6_route_) return;
    const int step = observation.step;
    if (step < 413 || step > 421) return;
    if (action.n_units <= 11) {
        ++stats_.route_failures;
        target6_route_ = false;
        return;
    }
    constexpr std::array<kag::UnitAction, 9> route = {{
        {kag::OP_WEST, 0, 1}, {kag::OP_DIG, 0, 1},
        {kag::OP_SOUTH, 0, 1}, {kag::OP_WEST, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_WEST, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_PLANT, kag::MELON, 1},
        {kag::OP_WATER, 0, 1},
    }};
    action.units[11] = route[static_cast<size_t>(step - 413)];
    action.finalize();
}

void Overlay::modify(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    if (observation.player != player_) std::abort();
    verify_route(observation);
    apply_target6_route(observation, action);
    if (!(parameters_.suppress_day_mask & (uint32_t{1} << observation.day)))
        return;
    int excess = hires(action) - nominal_hires(observation.step);
    if (observation.day != 17 ||
        (parameters_.suppress_hires ? excess <= 0 : observation.step != 409))
        return;
    int weed_count = 0;
    int target_index = -1;
    for (int index = 0; index < static_cast<int>(TARGETS.size()); ++index) {
        const Target& target = TARGETS[index];
        if (observation.self().tiles[target.y][target.x].kind != kag::T_WEED)
            continue;
        ++weed_count;
        target_index = index;
    }
    if ((parameters_.require_single_target && weed_count != 1) ||
        target_index < 0 ||
        !(parameters_.target_mask & (uint16_t{1} << target_index)))
        return;

    if (parameters_.repair_target1 && target_index == 1 && action.n_units > 2 &&
        observation.self().pos_x[2] == TARGETS[1].x &&
        observation.self().pos_y[2] == TARGETS[1].y &&
        action.units[2].op == kag::OP_PASS) {
        action.units[2] = {kag::OP_DIG, 0, 1};
        target1_check_ = true;
        ++stats_.target1_repairs;
    }
    if (parameters_.repair_target6 && target_index == 6) {
        if (observation.self().tiles[8][0].kind == kag::T_WEED)
            ++stats_.target6_companion_fallbacks;
        else {
            target6_route_ = true;
            ++stats_.target6_repairs;
        }
    }
    if (parameters_.suppress_hires) {
        while (excess-- > 0) {
            int remove = action.n_orders - 1;
            while (remove >= 0 && action.orders[remove].op != kag::M_HIRE)
                --remove;
            if (remove < 0) std::abort();
            for (int order = remove + 1; order < action.n_orders; ++order)
                action.orders[order - 1] = action.orders[order];
            --action.n_orders;
            ++stats_.suppressed_hires;
        }
    }
    suppressed_target_ = target_index;
    action.finalize();
}

}  // namespace kag::agents::brunch_114643_robust::detail::residual_route
