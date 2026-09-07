#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/reassignment_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::brunch_114643_robust::detail::reassignment {
namespace tape {
#ifndef FIXED_WEED_REASSIGNMENT_TAPE_INCLUDE
#define FIXED_WEED_REASSIGNMENT_TAPE_INCLUDE "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/tape.inc"
#endif
#include FIXED_WEED_REASSIGNMENT_TAPE_INCLUDE
}
namespace {

struct Commitment {
    int16_t step = 0;
    int8_t x = 0;
    int8_t y = 0;
    int8_t unit = 0;
    uint8_t op = 0;
    uint8_t arg = 0;
};

struct NominalTable {
    std::array<std::array<int8_t, kag::MAX_UNITS>, tape::TAPE_STEPS> x{};
    std::array<std::array<int8_t, kag::MAX_UNITS>, tape::TAPE_STEPS> y{};
    std::array<std::array<kag::UnitAction, kag::MAX_UNITS>, tape::TAPE_STEPS> actions{};
    std::array<uint8_t, tape::TAPE_STEPS> units{};
    std::array<int16_t, 30> last_hire_step{};
    std::array<Commitment, 256> commitments{};
    int commitment_count = 0;
};

kag::Action nominal_action(int step, int live_units) {
    kag::Action action;
    action.clear();
    action.n_units = live_units;
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    for (int unit = 0; unit < units; ++unit) {
        const kag::UnitAction selected = {
            static_cast<uint8_t>(tape::TAPE_DATA[cursor]),
            static_cast<uint8_t>(tape::TAPE_DATA[cursor + 1]),
            tape::TAPE_DATA[cursor + 2]};
        cursor += 3;
        if (unit < live_units) action.units[unit] = selected;
    }
    action.n_orders = std::min(orders, 10);
    for (int order = 0; order < orders; ++order) {
        action.orders[order] = {
            static_cast<uint8_t>(tape::TAPE_DATA[cursor]),
            static_cast<uint8_t>(tape::TAPE_DATA[cursor + 1]),
            tape::TAPE_DATA[cursor + 2]};
        cursor += 3;
    }
    action.finalize();
    return action;
}

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
        for (int step = 0; step < tape::TAPE_STEPS; ++step) {
            const kag::Farm& farm = sim.st.farms[0];
            result.units[step] = static_cast<uint8_t>(farm.n_units);
            const kag::Action action = nominal_action(step, farm.n_units);
            for (int unit = 0; unit < farm.n_units; ++unit) {
                result.x[step][unit] = farm.pos_x[unit];
                result.y[step][unit] = farm.pos_y[unit];
                result.actions[step][unit] = action.units[unit];
                const uint8_t op = action.units[unit].op;
                if (op != kag::OP_PLANT && op != kag::OP_BUILD_COOP &&
                    op != kag::OP_BUILD_PASTURE)
                    continue;
                if (result.commitment_count >= static_cast<int>(result.commitments.size()))
                    std::abort();
                result.commitments[result.commitment_count++] = {
                    static_cast<int16_t>(step), farm.pos_x[unit], farm.pos_y[unit],
                    static_cast<int8_t>(unit), op, action.units[unit].arg};
            }
            for (int order = 0; order < action.n_orders; ++order)
                if (action.orders[order].op == kag::M_HIRE)
                    result.last_hire_step[step / 24] = static_cast<int16_t>(step);
            sim.step(action, pass);
        }
        return result;
    }();
    return table;
}

bool movement_or_pass(uint8_t op) {
    return op == kag::OP_PASS || (op >= kag::OP_NORTH && op <= kag::OP_WEST);
}

int distance(int x0, int y0, int x1, int y1) {
    return std::abs(x0 - x1) + std::abs(y0 - y1);
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

bool refined_target(const Commitment& commitment) {
    if (commitment.step / 24 == 11) {
        switch (commitment.step) {
            case 273: case 276: case 283: case 286:
                return commitment.op == kag::OP_PLANT && commitment.arg == kag::WHEAT;
            case 280:
                return commitment.op == kag::OP_BUILD_COOP ||
                    (commitment.op == kag::OP_PLANT && commitment.arg == kag::WHEAT);
            default: return false;
        }
    }
    if (commitment.step / 24 == 19)
        return (commitment.step == 473 && commitment.op == kag::OP_PLANT &&
                commitment.arg == kag::MELON) ||
            (commitment.step == 476 && commitment.op == kag::OP_PLANT &&
                commitment.arg == kag::WHEAT);
    const int index = day17_target_index(commitment);
    return index >= 0 && (uint16_t{25947} & (uint16_t{1} << index));
}

int nominal_hires(int step) {
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    cursor += units * 3;
    int result = 0;
    for (int order = 0; order < orders; ++order) {
        result += tape::TAPE_DATA[cursor] == kag::M_HIRE;
        cursor += 3;
    }
    return result;
}

std::array<int, 2> extra_spawn(int step) {
    const NominalTable& table = nominal_table();
    int access[4][2];
    kag::shed_access_tiles(kag::BOARD, access);
    int occupancy[4]{};
    for (int unit = 0; unit < table.units[step]; ++unit)
        for (int tile = 0; tile < 4; ++tile)
            if (table.x[step][unit] == access[tile][0] &&
                table.y[step][unit] == access[tile][1])
                ++occupancy[tile];
    int spawn = 0;
    const int accepted_hires = nominal_hires(step);
    for (int hire = 0; hire <= accepted_hires; ++hire) {
        spawn = 0;
        for (int tile = 1; tile < 4; ++tile)
            if (occupancy[tile] < occupancy[spawn]) spawn = tile;
        ++occupancy[spawn];
    }
    return {access[spawn][0], access[spawn][1]};
}

struct Target {
    int16_t deadline = 0;
    int8_t x = 0;
    int8_t y = 0;
};

struct Event {
    int16_t deadline = 0;
    int16_t release = 0;
    int8_t x = 0;
    int8_t y = 0;
    kag::UnitAction action{};
    bool target = false;
};

struct Candidate {
    std::array<kag::UnitAction, 24> actions{};
    std::array<int8_t, 24> x{};
    std::array<int8_t, 24> y{};
    int16_t start = 0;
    uint8_t size = 0;
    int cost = std::numeric_limits<int>::max();
    int shifted = 0;
    bool valid = false;
};

void append_move(Candidate& candidate, int& cursor, int& x, int& y, int tx, int ty) {
    while (x != tx) {
        candidate.x[cursor] = static_cast<int8_t>(x);
        candidate.y[cursor] = static_cast<int8_t>(y);
        candidate.actions[cursor++] = {
            static_cast<uint8_t>(x < tx ? kag::OP_EAST : kag::OP_WEST), 0, 1};
        x += x < tx ? 1 : -1;
    }
    while (y != ty) {
        candidate.x[cursor] = static_cast<int8_t>(x);
        candidate.y[cursor] = static_cast<int8_t>(y);
        candidate.actions[cursor++] = {
            static_cast<uint8_t>(y < ty ? kag::OP_SOUTH : kag::OP_NORTH), 0, 1};
        y += y < ty ? 1 : -1;
    }
}

Candidate build_candidate(const kag::agent::AgentObservation& observation, int unit,
                          const Target& target, int maximum_early) {
    const NominalTable& table = nominal_table();
    const int day = observation.day;
    const int day_start = day * 24;
    const int checkpoint = day_start + 23;
    int start = std::max(day_start,
        static_cast<int>(table.last_hire_step[day]) + 1);
    while (start < checkpoint && unit >= table.units[start]) ++start;
    Candidate best;
    if (start >= checkpoint || start >= target.deadline) return best;
    for (int index = 0; index < table.commitment_count; ++index) {
        const Commitment& commitment = table.commitments[index];
        if (commitment.unit != unit || commitment.step < start ||
            commitment.step >= checkpoint ||
            observation.self().tiles[commitment.y][commitment.x].kind != kag::T_WEED)
            continue;
        if (commitment.x != target.x || commitment.y != target.y) return best;
    }

    std::array<Event, 25> services{};
    int service_count = 0;
    for (int step = start; step < checkpoint; ++step) {
        const kag::UnitAction action = table.actions[step][unit];
        if (movement_or_pass(action.op)) continue;
        services[service_count++] = {
            static_cast<int16_t>(step),
            static_cast<int16_t>(std::max(start, step - maximum_early)),
            table.x[step][unit], table.y[step][unit], action, false};
    }

    for (int insertion = 0; insertion <= service_count; ++insertion) {
        std::array<Event, 26> events{};
        int event_count = 0;
        for (int index = 0; index <= service_count; ++index) {
            if (index == insertion)
                events[event_count++] = {static_cast<int16_t>(target.deadline - 1),
                    static_cast<int16_t>(start), target.x, target.y,
                    {kag::OP_DIG, 0, 1}, true};
            if (index < service_count) events[event_count++] = services[index];
        }

        std::array<int16_t, 26> times{};
        int next_time = checkpoint;
        int next_x = table.x[checkpoint][unit];
        int next_y = table.y[checkpoint][unit];
        bool feasible = true;
        for (int index = event_count - 1; index >= 0; --index) {
            const Event& event = events[index];
            times[index] = static_cast<int16_t>(std::min<int>(event.deadline,
                next_time - distance(event.x, event.y, next_x, next_y) - 1));
            if (times[index] < event.release) {
                feasible = false;
                break;
            }
            next_time = times[index];
            next_x = event.x;
            next_y = event.y;
        }
        if (!feasible || start + distance(table.x[start][unit], table.y[start][unit],
                events[0].x, events[0].y) > times[0])
            continue;

        Candidate candidate;
        candidate.start = static_cast<int16_t>(start);
        int cursor = 0;
        int time = start;
        int x = table.x[start][unit];
        int y = table.y[start][unit];
        int shifted = 0;
        for (int index = 0; index < event_count; ++index) {
            const Event& event = events[index];
            const int travel = distance(x, y, event.x, event.y);
            while (time < times[index] - travel) {
                candidate.x[cursor] = static_cast<int8_t>(x);
                candidate.y[cursor] = static_cast<int8_t>(y);
                candidate.actions[cursor++] = {};
                ++time;
            }
            append_move(candidate, cursor, x, y, event.x, event.y);
            time += travel;
            candidate.x[cursor] = static_cast<int8_t>(x);
            candidate.y[cursor] = static_cast<int8_t>(y);
            candidate.actions[cursor++] = event.action;
            ++time;
            if (!event.target) shifted += event.deadline - times[index];
        }
        const int end_travel = distance(x, y, table.x[checkpoint][unit],
            table.y[checkpoint][unit]);
        append_move(candidate, cursor, x, y, table.x[checkpoint][unit],
            table.y[checkpoint][unit]);
        time += end_travel;
        while (time < checkpoint) {
            candidate.x[cursor] = static_cast<int8_t>(x);
            candidate.y[cursor] = static_cast<int8_t>(y);
            candidate.actions[cursor++] = {};
            ++time;
        }
        if (cursor != checkpoint - start || cursor > 24)
            std::abort();
        int changed = 0;
        for (int offset = 0; offset < cursor; ++offset) {
            const kag::UnitAction a = candidate.actions[offset];
            const kag::UnitAction b = table.actions[start + offset][unit];
            changed += a.op != b.op || a.arg != b.arg || a.n != b.n;
        }
        candidate.size = static_cast<uint8_t>(cursor);
        candidate.cost = 100 * shifted + changed;
        candidate.shifted = shifted;
        candidate.valid = true;
        if (!best.valid || candidate.cost < best.cost) best = candidate;
    }
    return best;
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    last_day_ = -1;
    stats_ = {};
    matched_day_ = false;
    for (Route& route : routes_) route = {};
}

void Overlay::begin_day(const kag::agent::AgentObservation& observation) {
    last_day_ = observation.day;
    matched_day_ = false;
    for (Route& route : routes_) route = {};
    if (!parameters_.enabled ||
        !(parameters_.day_mask & (uint32_t{1} << observation.day))) return;

    std::array<Target, MAX_TARGETS> targets{};
    int target_count = 0;
    const NominalTable& table = nominal_table();
    for (int index = 0; index < table.commitment_count; ++index) {
        const Commitment& commitment = table.commitments[index];
        if (commitment.step / 24 != observation.day || !refined_target(commitment) ||
            observation.self().tiles[commitment.y][commitment.x].kind != kag::T_WEED)
            continue;
        int existing = -1;
        for (int target = 0; target < target_count; ++target)
            if (targets[target].x == commitment.x && targets[target].y == commitment.y)
                existing = target;
        if (existing >= 0) {
            targets[existing].deadline = std::min<int16_t>(targets[existing].deadline,
                commitment.step);
        } else {
            if (target_count >= MAX_TARGETS) std::abort();
            targets[target_count++] = {commitment.step, commitment.x, commitment.y};
        }
    }
    stats_.observed_targets += target_count;
    if (!target_count) return;

    const int hire_step = table.last_hire_step[observation.day];
    if (hire_step < 0) return;
    const std::array<int, 2> spawn = extra_spawn(hire_step);
    std::array<Target, MAX_TARGETS> required_targets{};
    int required_count = 0;
    for (int target = 0; target < target_count; ++target) {
        const int travel = distance(spawn[0], spawn[1], targets[target].x,
            targets[target].y);
        if (hire_step + 1 + travel < targets[target].deadline)
            required_targets[required_count++] = targets[target];
    }
    targets = required_targets;
    target_count = required_count;
    if (!target_count) return;

    std::array<std::array<Candidate, kag::MAX_UNITS>, MAX_TARGETS> candidates{};
    for (int target = 0; target < target_count; ++target)
        for (int unit = 0; unit < kag::MAX_UNITS; ++unit)
            candidates[target][unit] = build_candidate(observation, unit, targets[target],
                parameters_.maximum_early);

    std::array<int8_t, MAX_TARGETS> best_assignment{};
    best_assignment.fill(-1);
    static constexpr int MAX_MATCH_UNITS = 12;
    static constexpr int MATCH_STATES = 1 << MAX_MATCH_UNITS;
    const int checkpoint = observation.day * 24 + 23;
    const int unit_count = table.units[checkpoint];
    if (unit_count > MAX_MATCH_UNITS) std::abort();
    const int states = 1 << unit_count;
    constexpr int INF = std::numeric_limits<int>::max() / 4;
    std::array<int, MATCH_STATES> costs{};
    std::array<int, MATCH_STATES> next_costs{};
    std::array<std::array<int16_t, MATCH_STATES>, MAX_TARGETS + 1> parent_mask{};
    std::array<std::array<int8_t, MATCH_STATES>, MAX_TARGETS + 1> parent_unit{};
    costs.fill(INF);
    costs[0] = 0;
    for (int target = 0; target < target_count; ++target) {
        next_costs.fill(INF);
        for (int mask = 0; mask < states; ++mask) {
            if (costs[mask] == INF) continue;
            for (int unit = 0; unit < unit_count; ++unit) {
                if ((mask & (1 << unit)) || !candidates[target][unit].valid) continue;
                const int next_mask = mask | (1 << unit);
                const int cost = costs[mask] + candidates[target][unit].cost;
                if (cost >= next_costs[next_mask]) continue;
                next_costs[next_mask] = cost;
                parent_mask[target + 1][next_mask] = static_cast<int16_t>(mask);
                parent_unit[target + 1][next_mask] = static_cast<int8_t>(unit);
            }
        }
        costs = next_costs;
    }
    int best_mask = -1;
    int best_cost = INF;
    for (int mask = 0; mask < states; ++mask)
        if (costs[mask] < best_cost) {
            best_cost = costs[mask];
            best_mask = mask;
        }
    if (best_mask < 0) return;
    for (int target = target_count; target > 0; --target) {
        best_assignment[target - 1] = parent_unit[target][best_mask];
        best_mask = parent_mask[target][best_mask];
    }

    for (int target = 0; target < target_count; ++target) {
        const int unit = best_assignment[target];
        const Candidate& candidate = candidates[target][unit];
        Route& route = routes_[unit];
        route.actions = candidate.actions;
        route.x = candidate.x;
        route.y = candidate.y;
        route.start = candidate.start;
        route.size = candidate.size;
        route.active = true;
        route.target_step = targets[target].deadline;
        route.target_x = targets[target].x;
        route.target_y = targets[target].y;
        stats_.route_actions += route.size;
        stats_.shifted_services += candidate.shifted;
        if (stats_.event_count < static_cast<int>(stats_.events.size()))
            stats_.events[stats_.event_count++] = {
                targets[target].deadline, candidate.start, targets[target].x,
                targets[target].y, static_cast<int8_t>(unit)};
    }
    stats_.covered_targets += target_count;
    ++stats_.matched_days;
    matched_day_ = true;
}

void Overlay::modify(const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (observation.player != player_) std::abort();
    if (observation.day != last_day_) begin_day(observation);
    if (!parameters_.enabled || !matched_day_) return;

    for (int unit = 0; unit < action.n_units; ++unit) {
        Route& route = routes_[unit];
        if (!route.active || observation.step < route.start ||
            observation.step >= route.start + route.size)
            continue;
        const int offset = observation.step - route.start;
        if (observation.self().pos_x[unit] != route.x[offset] ||
            observation.self().pos_y[unit] != route.y[offset]) {
            route.active = false;
            ++stats_.route_aborts;
            stats_.abort_step = observation.step;
            stats_.abort_unit = unit;
            stats_.abort_expected_x = route.x[offset];
            stats_.abort_expected_y = route.y[offset];
            stats_.abort_actual_x = observation.self().pos_x[unit];
            stats_.abort_actual_y = observation.self().pos_y[unit];
            continue;
        }
        kag::UnitAction selected = route.actions[offset];
        if (selected.op == kag::OP_DIG) {
            const kag::Tile& tile = observation.self().tiles[route.y[offset]][route.x[offset]];
            if (tile.kind != kag::T_WEED) {
                selected = {};
                ++stats_.redundant_digs;
            } else {
                ++stats_.route_digs;
            }
        }
        action.units[unit] = selected;
    }

    int action_hires = 0;
    for (int order = 0; order < action.n_orders; ++order)
        action_hires += action.orders[order].op == kag::M_HIRE;
    if (action_hires > nominal_hires(observation.step)) {
        int remove = action.n_orders - 1;
        while (remove >= 0 && action.orders[remove].op != kag::M_HIRE) --remove;
        if (remove < 0) std::abort();
        for (int order = remove + 1; order < action.n_orders; ++order)
            action.orders[order - 1] = action.orders[order];
        --action.n_orders;
        ++stats_.suppressed_hires;
    }
    action.finalize();
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::brunch_114643_robust::detail::reassignment
