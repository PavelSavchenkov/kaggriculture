#include "cleanup_route_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::brunch_114643_robust::detail::cleanup {
namespace tape {
#ifndef FIXED_WEED_RESIDUAL_TAPE_INCLUDE
#define FIXED_WEED_RESIDUAL_TAPE_INCLUDE \
    "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/brunch_114643_robust/tape.inc"
#endif
#include FIXED_WEED_RESIDUAL_TAPE_INCLUDE
}
namespace {

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

bool sole_weed(const kag::agent::AgentObservation& observation, int target_x,
               int target_y) {
    int weeds = 0;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            weeds += observation.self().tiles[y][x].kind == kag::T_WEED;
    return weeds == 1 &&
        observation.self().tiles[target_y][target_x].kind == kag::T_WEED;
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    route265_active_ = false;
    route121_active_ = false;
    route241_active_ = false;
    route457_active_ = false;
    stats_ = {};
}

void Overlay::apply_route241(
        const kag::agent::AgentObservation& observation,
        kag::Action& action) {
    if (!route241_active_) return;
    const int step = observation.step;
    if (step < 255) return;
    if (step > 260 || action.n_units <= 3) {
        ++stats_.aborts;
        route241_active_ = false;
        return;
    }
    constexpr std::array<int8_t, 6> X = {8, 8, 8, 8, 9, 9};
    constexpr std::array<kag::UnitAction, 6> ROUTE = {{
        {kag::OP_DIG, 0, 1}, {kag::OP_PLANT, kag::WHEAT, 1},
        {kag::OP_WATER, 0, 1}, {kag::OP_EAST, 0, 1},
        {kag::OP_PLANT, kag::WHEAT, 1}, {kag::OP_WATER, 0, 1},
    }};
    const size_t index = static_cast<size_t>(step - 255);
    bool expected = observation.self().pos_x[3] == X[index] &&
        observation.self().pos_y[3] == 2;
    const auto& first = observation.self().tiles[2][8];
    const auto& second = observation.self().tiles[2][9];
    if (step == 255) expected = expected && first.kind == kag::T_WEED;
    if (step == 256) expected = expected && first.kind == kag::T_EMPTY;
    if (step == 257)
        expected = expected && first.kind == kag::T_PLANT &&
            first.what == kag::WHEAT;
    if (step == 258) expected = expected && first.watered_today;
    if (step == 259) expected = expected && second.kind == kag::T_EMPTY;
    if (step == 260)
        expected = expected && second.kind == kag::T_PLANT &&
            second.what == kag::WHEAT;
    if (!expected) {
        ++stats_.aborts;
        route241_active_ = false;
        return;
    }
    action.units[3] = ROUTE[index];
    action.finalize();
    if (step == 260) {
        ++stats_.completed;
        route241_active_ = false;
    }
}

void Overlay::apply_route121(
        const kag::agent::AgentObservation& observation,
        kag::Action& action) {
    if (!route121_active_) return;
    const int step = observation.step;
    if (step < 134) return;
    if (step > 143 || action.n_units <= 6) {
        ++stats_.aborts;
        route121_active_ = false;
        return;
    }
    constexpr std::array<int8_t, 10> X0 =
        {4, 4, 4, 4, 4, 4, 4, 3, 3, 3};
    constexpr std::array<int8_t, 10> Y0 =
        {1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
    constexpr std::array<kag::UnitAction, 10> ROUTE0 = {{
        {kag::OP_DIG, 0, 1}, {kag::OP_BUILD_COOP, 0, 1},
        {kag::OP_PLACE, kag::GOOSE, 1}, {kag::OP_FEED, 0, 1},
        {kag::OP_CARE, 0, 1}, {kag::OP_NORTH, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_WATER, 0, 1},
        {kag::OP_HARVEST, 0, 1}, {kag::OP_PLANT, kag::CARROT, 1},
    }};
    const size_t index = static_cast<size_t>(step - 134);
    bool expected = observation.self().pos_x[0] == X0[index] &&
        observation.self().pos_y[0] == Y0[index];
    if (step >= 141)
        expected = expected && observation.self().pos_x[6] == step - 140 &&
            observation.self().pos_y[6] == 0;
    const auto& coop = observation.self().tiles[1][4];
    const auto& crop = observation.self().tiles[0][3];
    if (step == 134) expected = expected && coop.kind == kag::T_WEED;
    if (step == 135) expected = expected && coop.kind == kag::T_EMPTY;
    if (step == 136)
        expected = expected && coop.kind == kag::T_COOP && !coop.has_animal;
    if (step == 137)
        expected = expected && coop.kind == kag::T_COOP &&
            coop.has_animal && coop.what == kag::GOOSE && !coop.fed_today;
    if (step == 138)
        expected = expected && coop.has_animal && coop.fed_today &&
            !coop.cared_today;
    if (step == 139)
        expected = expected && coop.has_animal && coop.cared_today;
    if (step == 141 || step == 142)
        expected = expected && crop.kind == kag::T_PLANT;
    if (step == 143) expected = expected && crop.kind == kag::T_EMPTY;
    if (!expected) {
        ++stats_.aborts;
        route121_active_ = false;
        return;
    }
    action.units[0] = ROUTE0[index];
    if (step >= 141) {
        constexpr std::array<kag::UnitAction, 3> ROUTE6 = {{
            {kag::OP_EAST, 0, 1}, {kag::OP_EAST, 0, 1},
            {kag::OP_WATER, 0, 1},
        }};
        action.units[6] = ROUTE6[static_cast<size_t>(step - 141)];
    }
    action.finalize();
    if (step == 143) {
        ++stats_.completed;
        route121_active_ = false;
    }
}

void Overlay::apply_route265(
        const kag::agent::AgentObservation& observation,
        kag::Action& action) {
    if (!route265_active_) return;
    const int step = observation.step;
    if (step < 273) return;
    if (step > 287 || action.n_units <= 6) {
        ++stats_.aborts;
        route265_active_ = false;
        return;
    }
    constexpr std::array<int8_t, 15> X3 =
        {5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 6, 6, 6, 5};
    constexpr std::array<int8_t, 15> Y3 =
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    constexpr std::array<kag::UnitAction, 15> ROUTE3 = {{
        {kag::OP_DIG, 0, 1}, {kag::OP_PLANT, kag::WHEAT, 1},
        {kag::OP_WATER, 0, 1}, {kag::OP_EAST, 0, 1},
        {kag::OP_PLANT, kag::WHEAT, 1}, {kag::OP_WATER, 0, 1},
        {kag::OP_EAST, 0, 1}, {kag::OP_NORTH, 0, 1},
        {kag::OP_PLANT, kag::WHEAT, 1}, {kag::OP_WATER, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_PLANT, kag::WHEAT, 1},
        {kag::OP_WATER, 0, 1}, {kag::OP_WEST, 0, 1},
        {kag::OP_WATER, 0, 1},
    }};
    const size_t index = static_cast<size_t>(step - 273);
    bool expected = observation.self().pos_x[3] == X3[index] &&
        observation.self().pos_y[3] == Y3[index];
    if (step >= 279)
        expected = expected && observation.self().pos_x[2] ==
            (step == 279 ? 3 : (step == 280 ? 4 : 5)) &&
            observation.self().pos_y[2] == 0;
    const auto& first = observation.self().tiles[1][5];
    const auto& second = observation.self().tiles[1][6];
    const auto& third = observation.self().tiles[0][7];
    const auto& fourth = observation.self().tiles[0][6];
    const auto& fifth = observation.self().tiles[0][5];
    if (step == 273) expected = expected && first.kind == kag::T_WEED;
    if (step == 274) expected = expected && first.kind == kag::T_EMPTY;
    if (step == 275)
        expected = expected && first.kind == kag::T_PLANT &&
            first.what == kag::WHEAT;
    if (step == 276)
        expected = expected && first.watered_today &&
            second.kind == kag::T_EMPTY;
    if (step == 278)
        expected = expected && second.kind == kag::T_PLANT &&
            second.what == kag::WHEAT;
    if (step == 279) expected = expected && second.watered_today;
    if (step == 281) expected = expected && third.kind == kag::T_EMPTY;
    if (step == 282)
        expected = expected && third.kind == kag::T_PLANT &&
            third.what == kag::WHEAT;
    if (step == 283)
        expected = expected && third.watered_today &&
            fourth.kind == kag::T_EMPTY;
    if (step == 285)
        expected = expected && fourth.kind == kag::T_PLANT &&
            fourth.what == kag::WHEAT;
    if (step == 286)
        expected = expected && fourth.watered_today &&
            fifth.kind == kag::T_EMPTY;
    if (step == 287)
        expected = expected && fifth.kind == kag::T_PLANT &&
            fifth.what == kag::WHEAT;
    if (!expected) {
        ++stats_.aborts;
        route265_active_ = false;
        return;
    }
    action.units[3] = ROUTE3[index];
    if (step >= 279) {
        constexpr std::array<kag::UnitAction, 9> ROUTE2 = {{
            {kag::OP_EAST, 0, 1}, {kag::OP_EAST, 0, 1},
            {kag::OP_PASS, 0, 1}, {kag::OP_PASS, 0, 1},
            {kag::OP_PASS, 0, 1}, {kag::OP_PASS, 0, 1},
            {kag::OP_PASS, 0, 1}, {kag::OP_PLANT, kag::WHEAT, 1},
            {kag::OP_PASS, 0, 1},
        }};
        action.units[2] = ROUTE2[static_cast<size_t>(step - 279)];
    }
    action.finalize();
    if (step == 287) {
        ++stats_.completed;
        route265_active_ = false;
    }
}

void Overlay::apply_route457(
        const kag::agent::AgentObservation& observation,
        kag::Action& action) {
    if (!route457_active_) return;
    const int step = observation.step;
    if (step < 470) return;
    if (step > 479 || action.n_units <= 6) {
        ++stats_.aborts;
        route457_active_ = false;
        return;
    }
    constexpr std::array<int8_t, 10> X3 =
        {2, 2, 2, 2, 2, 2, 2, 1, 1, 1};
    constexpr std::array<int8_t, 10> Y3 =
        {8, 9, 9, 8, 7, 7, 7, 7, 7, 7};
    constexpr std::array<kag::UnitAction, 10> ROUTE3 = {{
        {kag::OP_SOUTH, 0, 1}, {kag::OP_DIG, 0, 1},
        {kag::OP_NORTH, 0, 1}, {kag::OP_NORTH, 0, 1},
        {kag::OP_PLANT, kag::MELON, 1}, {kag::OP_WATER, 0, 1},
        {kag::OP_WEST, 0, 1}, {kag::OP_PLANT, kag::MELON, 1},
        {kag::OP_WATER, 0, 1}, {kag::OP_PASS, 0, 1},
    }};
    const size_t index = static_cast<size_t>(step - 470);
    bool expected = observation.self().pos_x[3] == X3[index] &&
        observation.self().pos_y[3] == Y3[index];
    if (step >= 473)
        expected = expected && observation.self().pos_x[6] ==
            (step == 473 ? 0 : 1) && observation.self().pos_y[6] == 8;
    const auto& target = observation.self().tiles[9][2];
    const auto& upper = observation.self().tiles[7][2];
    const auto& left = observation.self().tiles[7][1];
    const auto& lower = observation.self().tiles[8][1];
    if (step == 470 || step == 471)
        expected = expected && target.kind == kag::T_WEED;
    else if (step <= 473)
        expected = expected && target.kind == kag::T_EMPTY;
    if (step == 474)
        expected = expected && upper.kind == kag::T_EMPTY &&
            lower.kind == kag::T_EMPTY;
    if (step == 475)
        expected = expected && upper.kind == kag::T_PLANT &&
            upper.what == kag::MELON && lower.kind == kag::T_PLANT &&
            lower.what == kag::MELON;
    if (step == 477)
        expected = expected && left.kind == kag::T_EMPTY;
    if (step == 478)
        expected = expected && left.kind == kag::T_PLANT &&
            left.what == kag::MELON;
    if (step == 479)
        expected = expected && upper.kind == kag::T_PLANT &&
            upper.watered_today && left.kind == kag::T_PLANT &&
            left.watered_today && lower.kind == kag::T_PLANT &&
            lower.watered_today && target.kind == kag::T_PLANT &&
            target.what == kag::MELON && target.watered_today;
    if (!expected) {
        ++stats_.aborts;
        route457_active_ = false;
        return;
    }
    action.units[3] = ROUTE3[index];
    if (step >= 473) {
        constexpr std::array<kag::UnitAction, 7> ROUTE6 = {{
            {kag::OP_EAST, 0, 1}, {kag::OP_PLANT, kag::MELON, 1},
            {kag::OP_WATER, 0, 1}, {kag::OP_PASS, 0, 1},
            {kag::OP_PASS, 0, 1}, {kag::OP_PASS, 0, 1},
            {kag::OP_PASS, 0, 1},
        }};
        action.units[6] = ROUTE6[static_cast<size_t>(step - 473)];
    }
    action.finalize();
    if (step == 479) {
        ++stats_.completed;
        route457_active_ = false;
    }
}

void Overlay::apply_overflow599(
        const kag::agent::AgentObservation& observation,
        kag::Action& action) {
    if (!overflow599_enabled_ || observation.step != 599 ||
        action.n_orders >= 10)
        return;
    int inventory = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int item = 0; item < kag::N_ITEMS; ++item)
            inventory += observation.own.inv[unit][item];
    std::array<int, kag::N_ITEMS> sold{};
    for (int order = 0; order < action.n_orders; ++order) {
        const kag::Order& value = action.orders[order];
        if (value.op != kag::M_SELL || value.item >= kag::N_PRODUCTS ||
            value.n <= 0)
            return;
        sold[value.item] += value.n;
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        inventory -= std::min<int>(observation.own.shed[item],
                                   sold[static_cast<size_t>(item)]);
    for (int unit = 0; unit < action.n_units; ++unit) {
        const int op = action.units[unit].op;
        if (op != kag::OP_PASS && op != kag::OP_WATER &&
            op != kag::OP_CARE)
            return;
    }
    const int overflow = inventory - 100;
    const int available_fertilizer = observation.own.shed[kag::FERTILIZER] -
        std::min<int>(observation.own.shed[kag::FERTILIZER],
                      sold[kag::FERTILIZER]);
    if (overflow <= 0 || available_fertilizer < overflow)
        return;
    action.orders[action.n_orders++] = {
        kag::M_SELL, kag::FERTILIZER, overflow};
    action.finalize();
    stats_.overflow_sales += overflow;
}

void Overlay::modify(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    if (observation.player != player_) std::abort();
    apply_route265(observation, action);
    apply_route121(observation, action);
    apply_route241(observation, action);
    apply_route457(observation, action);
    apply_overflow599(observation, action);
    const bool route265 = route265_enabled_ && observation.step == 265 &&
        sole_weed(observation, 5, 1);
    const bool route121 = route121_enabled_ && observation.step == 121 &&
        sole_weed(observation, 4, 1);
    const bool route241 = route241_enabled_ && observation.step == 241 &&
        observation.self().tiles[1][7].kind == kag::T_WEED &&
        observation.self().tiles[2][8].kind == kag::T_WEED &&
        !sole_weed(observation, 7, 1);
    const bool route457 = enabled_ && observation.step == 457 &&
        sole_weed(observation, 2, 9);
    if (route241) {
        int weeds = 0;
        for (int y = 0; y < kag::BOARD; ++y)
            for (int x = 0; x < kag::BOARD; ++x)
                weeds += observation.self().tiles[y][x].kind == kag::T_WEED;
        if (weeds != 2) return;
    }
    if (!route121 && !route241 && !route265 && !route457) return;
    const int excess = hires(action) - nominal_hires(observation.step);
    if (excess != 1) return;
    remove_last_hire(action);
    action.finalize();
    route265_active_ = route265;
    route121_active_ = route121;
    route241_active_ = route241;
    route457_active_ = route457;
    ++stats_.started;
    ++stats_.suppressed_hires;
}

}  // namespace kag::agents::brunch_114643_robust::detail::cleanup
