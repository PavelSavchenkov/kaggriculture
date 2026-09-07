#include "target8_overlay.hpp"

#include <array>
#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::four_random_shop::base::warm::bakery_108320_robust::detail::target8 {
namespace tape {
#ifndef FIXED_WEED_RESIDUAL_TAPE_INCLUDE
#define FIXED_WEED_RESIDUAL_TAPE_INCLUDE \
    "agents/inhouse/four_random_shop/source/base/warm/specialists/bakery_108320_robust/tape.inc"
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

void remove_last_hire(kag::Action& action) {
    int remove = action.n_orders - 1;
    while (remove >= 0 && action.orders[remove].op != kag::M_HIRE) --remove;
    if (remove < 0) std::abort();
    for (int order = remove + 1; order < action.n_orders; ++order)
        action.orders[order - 1] = action.orders[order];
    --action.n_orders;
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    route_active_ = false;
    route_companion_ = false;
    route_completion_pending_ = false;
    stats_ = {};
    decision_ = {};
}

void Overlay::apply_route(const kag::agent::AgentObservation& observation,
                          kag::Action& action) {
    if (!route_active_) return;
    const int step = observation.step;
    if (step == 426 && route_completion_pending_) {
        const auto& target = observation.self().tiles[9][4];
        const auto& companion = observation.self().tiles[8][0];
        const auto& melon = observation.self().tiles[9][0];
        if (target.kind == kag::T_PLANT && companion.kind == kag::T_EMPTY &&
            melon.kind == kag::T_PLANT && melon.what == kag::MELON)
            ++stats_.route_completed;
        else
            ++stats_.route_aborts;
        route_active_ = false;
        route_companion_ = false;
        route_completion_pending_ = false;
        return;
    }
    if (step == 425 && route_companion_) {
        const auto& target = observation.self().tiles[9][4];
        const auto& companion = observation.self().tiles[8][0];
        const auto& melon = observation.self().tiles[9][0];
        const bool rejoined = observation.self().n_units > 11 &&
            observation.self().pos_x[11] == 0 &&
            observation.self().pos_y[11] == 9 &&
            companion.kind == kag::T_EMPTY &&
            melon.kind == kag::T_PLANT && melon.what == kag::MELON &&
            melon.watered_today;
        if (rejoined && target.kind == kag::T_PLANT)
            ++stats_.route_completed;
        else if (rejoined && target.kind == kag::T_EMPTY) {
            route_completion_pending_ = true;
            return;
        }
        else
            ++stats_.route_aborts;
        route_active_ = false;
        route_companion_ = false;
        return;
    }
    if (step == 422 && !route_companion_) {
        const auto& target = observation.self().tiles[9][4];
        const auto& melon = observation.self().tiles[9][0];
        if (target.kind == kag::T_EMPTY && melon.kind == kag::T_PLANT &&
            melon.what == kag::MELON && melon.watered_today)
            ++stats_.route_completed;
        else
            ++stats_.route_aborts;
        route_active_ = false;
        return;
    }
    if (step < 415 || step > (route_companion_ ? 424 : 421)) return;
    if (action.n_units <= 11) {
        ++stats_.route_aborts;
        route_active_ = false;
        return;
    }
    constexpr std::array<int8_t, 10> x = {4, 4, 3, 2, 1, 0, 0, 0, 0, 0};
    constexpr std::array<int8_t, 10> y = {9, 9, 9, 9, 9, 9, 9, 9, 8, 8};
    const size_t index = static_cast<size_t>(step - 415);
    const bool expected_position = observation.self().pos_x[11] == x[index] &&
        observation.self().pos_y[11] == y[index];
    bool expected_tile = true;
    if (step == 415)
        expected_tile = observation.self().tiles[9][4].kind == kag::T_WEED;
    else if (step == 416)
        expected_tile = observation.self().tiles[9][4].kind == kag::T_EMPTY;
    else if (step == 420)
        expected_tile = observation.self().tiles[9][0].kind == kag::T_EMPTY;
    else if (step == 421) {
        const auto& tile = observation.self().tiles[9][0];
        expected_tile = tile.kind == kag::T_PLANT && tile.what == kag::MELON;
    } else if (step == 422)
        expected_tile = observation.self().tiles[9][0].kind == kag::T_PLANT;
    else if (step == 423)
        expected_tile = observation.self().tiles[8][0].kind == kag::T_WEED;
    else if (step == 424)
        expected_tile = observation.self().tiles[8][0].kind == kag::T_EMPTY;
    if (!expected_position || !expected_tile) {
        ++stats_.route_aborts;
        route_active_ = false;
        return;
    }
    constexpr std::array<kag::UnitAction, 10> ROUTE = {{
        {kag::OP_DIG, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_WEST, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_WEST, 0, 1},
        {kag::OP_PLANT, kag::MELON, 1}, {kag::OP_WATER, 0, 1},
        {kag::OP_NORTH, 0, 1}, {kag::OP_DIG, 0, 1},
        {kag::OP_SOUTH, 0, 1},
    }};
    action.units[11] = ROUTE[index];
    action.finalize();
}

void Overlay::modify(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    if (observation.player != player_) std::abort();
    apply_route(observation, action);
    if (mode_ == Mode::off || observation.step != 409) return;
    int excess = hires(action) - nominal_hires(observation.step);
    if (excess <= 0) return;

    Decision value;
    value.step = observation.step;
    value.cash = static_cast<int>(observation.self().money);
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            if (observation.self().tiles[y][x].kind != kag::T_WEED) continue;
            ++value.all_weed_count;
            const int tile = y * kag::BOARD + x;
            if (tile < 64) value.weed_lo |= uint64_t{1} << tile;
            else value.weed_hi |= uint64_t{1} << (tile - 64);
        }
    for (int index = 0; index < static_cast<int>(TARGETS.size()); ++index) {
        const auto& target = TARGETS[index];
        if (observation.self().tiles[target.y][target.x].kind != kag::T_WEED)
            continue;
        ++value.target_count;
        value.highest_target = index;
    }
    if (value.highest_target != 8) return;
    ++stats_.decisions;
    decision_ = value;
    if ((mode_ == Mode::route_endpoint_guard ||
         mode_ == Mode::route_companion_guard ||
         mode_ == Mode::route_closed || mode_ == Mode::route_safe) &&
        observation.self().tiles[9][0].kind == kag::T_WEED) {
        ++stats_.endpoint_fallbacks;
        return;
    }
    if (mode_ == Mode::route_companion_guard &&
        observation.self().tiles[8][0].kind == kag::T_WEED) {
        ++stats_.companion_fallbacks;
        return;
    }
    if (mode_ == Mode::route_safe &&
        observation.self().tiles[8][4].kind == kag::T_WEED) {
        ++stats_.conflict_fallbacks;
        return;
    }
    if ((mode_ == Mode::route_single_target ||
         mode_ == Mode::route_strict) && value.target_count != 1) {
        ++stats_.multiple_fallbacks;
        return;
    }
    while (excess-- > 0) {
        remove_last_hire(action);
        ++stats_.suppressed_hires;
    }
    decision_.triggered = true;
    if (mode_ != Mode::suppress) {
        route_active_ = true;
        route_companion_ = (mode_ == Mode::route_closed ||
            mode_ == Mode::route_safe || mode_ == Mode::route_strict) &&
            observation.self().tiles[8][0].kind == kag::T_WEED;
        stats_.companion_routes += route_companion_;
        ++stats_.route_started;
    }
    action.finalize();
}

}  // namespace kag::agents::four_random_shop::base::warm::bakery_108320_robust::detail::target8
