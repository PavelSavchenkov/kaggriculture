#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>

#include "problem_json.hpp"
#include "problem_validation.hpp"
#include "tile_graph.hpp"
#include "replay.hpp"

using namespace day_solver;

namespace {

using Schedule = std::array<kag::Action, HOURS>;

ManagedTileState state(ManagedTileKind kind) {
    ManagedTileState result;
    result.kind = kind;
    return result;
}

DayProblem problem(int quadrant, int hour, ManagedTileKind end) {
    DayProblem result;
    result.start.managed_tiles.push_back({
        static_cast<int8_t>(4 + (quadrant % 2)),
        static_cast<int8_t>(4 + (quadrant / 2)),
        state(ManagedTileKind::LOCKED)});
    result.required_end_tiles.push_back({0, end, -1, state(end)});
    result.market_plan.push_back({static_cast<int8_t>(hour), 2,
                                 kag::M_BUY_LAND, static_cast<int16_t>(quadrant), 1, 0});
    return result;
}

Schedule schedule(const DayProblem& input) {
    Schedule result{};
    for (const auto& event : input.market_plan) {
        auto& hour = result[event.hour];
        hour.n_orders = std::max<int>(hour.n_orders, event.order_index + 1);
        hour.orders[event.order_index] = {event.market_op, 0, 0};
    }
    return result;
}

bool accepts(const TileGraph& graph, int action_hour = HOURS) {
    int node = 0;
    for (int hour = 0; hour < HOURS; ++hour) {
        int next = -1;
        for (const auto& arc : graph.arcs)
            if (arc.from == node && arc.prefix == (hour >= action_hour)) {
                assert(next == -1);
                next = arc.to;
            }
        if (next < 0) return false;
        node = next;
    }
    return std::find(graph.terminals.begin(), graph.terminals.end(), node)
        != graph.terminals.end();
}

kag::Sim engine(const DayProblem& input, int initial_quadrants) {
    kag::Config config;
    config.weed_chance = 0;
    config.starting_money = 1000000;
    kag::Sim result(config);
    auto& farm = result.st.farms[0];
    farm.n_quadrants = initial_quadrants;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            if (kag::quadrant_of(x, y, kag::BOARD) < initial_quadrants) {
                farm.tiles[y][x].kind = kag::T_EMPTY;
                const int index = y * kag::BOARD + x;
                farm.empty_mask[index >> 6] |= uint64_t{1} << (index & 63);
            }
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        farm.shed[item] = input.start.shed[item];
        farm.shed_total += input.start.shed[item];
    }
    return result;
}

void compare(const DayProblem& input, Schedule actions, int initial,
             bool strict, bool requirements) {
    const auto parsed = parse_problem_json(serialize_problem_json(input));
    assert(validate_problem(parsed).empty());
    assert(parsed.start.managed_tiles == input.start.managed_tiles);
    const auto result = replay_schedule(parsed, actions);
    assert(result.candidate.replay.strict_valid == strict);
    assert(result.requirements_satisfied == requirements);
    if (strict && requirements) assert(result.invariants_satisfied);
    auto sim = engine(input, initial);
    for (auto& action : actions) {
        action.finalize();
        sim.step(action, kag::Action{});
    }
    const auto& farm = sim.st.farms[0];
    assert(farm.n_quadrants == initial + static_cast<int>(input.market_plan.size()));
    for (const auto& tile : result.candidate.end.physical.managed_tiles) {
        const auto kind = farm.tiles[tile.y][tile.x].kind;
        const auto expected = kind == kag::T_LOCKED ? ManagedTileKind::LOCKED
            : kind == kag::T_COOP ? ManagedTileKind::COOP : ManagedTileKind::EMPTY;
        assert(kind == kag::T_LOCKED || kind == kag::T_EMPTY || kind == kag::T_COOP);
        assert(tile.state == state(expected));
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        assert(result.candidate.end.physical.shed[item] == farm.shed[item]);
}

}  // namespace

int main() {
    int comparisons = 0;
    for (int quadrant = 1; quadrant <= 3; ++quadrant) {
        for (int bought = 0; bought < HOURS; ++bought) {
            auto input = problem(quadrant, bought, ManagedTileKind::COOP);
            input.tile_work = {{0, {{kag::OP_BUILD_COOP, -1, 1, -1, 0}}}};
            const auto graph = build_tile_graphs(input).at(0);
            const int earliest = quadrant == 3 ? 2 : 1;
            for (int worked = earliest; worked < HOURS; ++worked) {
                auto actions = schedule(input);
                actions[0].units[0].op = quadrant == 2 ? kag::OP_SOUTH : kag::OP_EAST;
                if (quadrant == 3) actions[1].units[0].op = kag::OP_SOUTH;
                actions[worked].units[0].op = kag::OP_BUILD_COOP;
                const bool legal = worked > bought;
                assert(accepts(graph, worked) == legal);
                compare(input, actions, quadrant, legal, legal);
                ++comparisons;
            }
            input.tile_work.clear();
            input.required_end_tiles = {{0, ManagedTileKind::EMPTY, -1, state(ManagedTileKind::EMPTY)}};
            assert(accepts(build_tile_graphs(input).at(0)));
            compare(input, schedule(input), quadrant, true, true);
            input.required_end_tiles = {{0, ManagedTileKind::LOCKED, -1, state(ManagedTileKind::LOCKED)}};
            assert(validate_problem(input).empty());  // Wrong endpoint is an infeasible request.
            assert(build_tile_graphs(input).at(0).terminals.empty());
            compare(input, schedule(input), quadrant, true, false);
            comparisons += 2;
        }
        // Travel, PICKUP, PLACE, and DROP remain legal before unlocking.
        auto input = problem(quadrant, 23, ManagedTileKind::EMPTY);
        input.start.shed[kag::WHEAT] = input.end_shed[kag::WHEAT] = 3;
        auto actions = schedule(input);
        actions[0].units[0].op = quadrant == 2 ? kag::OP_SOUTH : kag::OP_EAST;
        if (quadrant == 3) actions[1].units[0].op = kag::OP_SOUTH;
        actions[2].units[0] = {kag::OP_PICKUP, kag::WHEAT, 3};
        actions[3].units[0] = {kag::OP_PLACE, kag::WHEAT, 1};
        actions[4].units[0].op = kag::OP_DROP;
        compare(input, actions, quadrant, true, true);
        input.market_plan.clear();
        input.required_end_tiles = {{0, ManagedTileKind::LOCKED, -1, state(ManagedTileKind::LOCKED)}};
        actions[23].n_orders = 0;
        assert(accepts(build_tile_graphs(input).at(0)));
        compare(input, actions, quadrant, true, true);
        comparisons += 2;
    }
    for (int hour : {0, 23}) {
        auto input = problem(1, hour, ManagedTileKind::EMPTY);
        for (int quadrant : {2, 3}) {
            input.start.managed_tiles.push_back(problem(quadrant, hour, ManagedTileKind::EMPTY).start.managed_tiles[0]);
            input.required_end_tiles.push_back({static_cast<int16_t>(quadrant - 1),
                ManagedTileKind::EMPTY, -1, state(ManagedTileKind::EMPTY)});
            input.market_plan.push_back({static_cast<int8_t>(hour), static_cast<int8_t>(2 * quadrant),
                kag::M_BUY_LAND, static_cast<int16_t>(quadrant), 1, 0});
        }
        std::reverse(input.market_plan.begin(), input.market_plan.end());
        for (const auto& graph : build_tile_graphs(input)) assert(accepts(graph));
        compare(input, schedule(input), 1, true, true);
        ++comparisons;
    }
    const auto base = problem(1, 0, ManagedTileKind::EMPTY);
    for (int item : {-1, 0, 4}) {
        auto bad = base;
        bad.market_plan[0].item = item;
        assert(!validate_problem(bad).empty());
    }
    {
        auto bad = base;
        bad.market_plan[0].quantity = 2;
        assert(!validate_problem(bad).empty());
    }
    for (int next : {1, 3}) {
        auto bad = base;
        bad.market_plan.push_back({1, 0, kag::M_BUY_LAND, static_cast<int16_t>(next), 1, 0});
        assert(!validate_problem(bad).empty());
    }
    {
        auto bad = base;
        bad.start.managed_tiles[0].state = {};
        assert(!validate_problem(bad).empty());
        bad = base;
        bad.market_plan.clear();
        bad.start.managed_tiles.push_back({4, 5, {}});  // SW owned, NE locked.
        bad.required_end_tiles.push_back({1, ManagedTileKind::EMPTY, -1, ManagedTileState{}});
        assert(!validate_problem(bad).empty());
        bad = base;
        bad.start.managed_tiles[0].x = 4;  // NW cannot be locked.
        assert(!validate_problem(bad).empty());
        bad = base;
        bad.start.managed_tiles[0].state.stored_units = 1;
        assert(!validate_problem(bad).empty());
    }
    std::cout << comparisons << " land replay/graph/engine comparisons and malformed-input checks passed\n";
}
