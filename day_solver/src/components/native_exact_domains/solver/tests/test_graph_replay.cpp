#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "tile_graph.hpp"
#include "replay.hpp"

using namespace day_solver;

namespace {

struct Fixture {
    std::string name;
    ManagedTileState start;
    std::array<TileWorkAction, 2> work;
    int input = -1;
};

TileWorkAction action(int op, int arg = -1, int output = -1, int quantity = 0) {
    return {static_cast<uint8_t>(op), static_cast<int16_t>(arg), 1,
            static_cast<int16_t>(output), quantity};
}

bool accepts(const TileGraph& graph, int first, int second) {
    int node = 0;
    for (int hour = 0; hour < HOURS; ++hour) {
        const int prefix = (hour >= first) + (hour >= second);
        int target = -1;
        for (const auto& arc : graph.arcs)
            if (arc.from == node && arc.prefix == prefix) {
                assert(target == -1);
                target = arc.to;
            }
        if (target < 0) return false;
        node = target;
    }
    return std::find(graph.terminals.begin(), graph.terminals.end(), node)
        != graph.terminals.end();
}

auto schedule(const Fixture& fixture, int first, int second) {
    std::array<kag::Action, HOURS> result{};
    if (fixture.input >= 0)
        result[0].units[0] = {kag::OP_PICKUP,
                             static_cast<uint8_t>(fixture.input), 1};
    const std::array<int, 2> hours{first, second};
    for (int index = 0; index < 2; ++index) {
        const auto& work = fixture.work[index];
        result[hours[index]].units[0] = {
            work.op, static_cast<uint8_t>(std::max<int>(0, work.arg)), 1};
    }
    return result;
}

EndTileRequirement end_requirement(const ManagedTileState& state) {
    const int item = state.kind == ManagedTileKind::CROP ? state.crop
        : animal_structure(state.kind) ? state.animal : NO_SUBJECT;
    return {0, state.kind, static_cast<int16_t>(item), state};
}

int check_fixture(const Fixture& fixture) {
    DayProblem problem;
    problem.start.managed_tiles.push_back({4, 4, fixture.start});
    problem.tile_work.push_back({0, {fixture.work.begin(), fixture.work.end()}});
    if (fixture.input >= 0) problem.start.shed[fixture.input] = 1;
    problem.end_shed = problem.start.shed;
    problem.required_end_tiles = {end_requirement(fixture.start)};
    const int earliest = fixture.input >= 0 ? 1 : 0;

    // Build a test target from the first strict full replay. The graph builder
    // sees only the resulting DayProblem, never this schedule.
    bool found = false;
    for (int first = earliest; first < HOURS && !found; ++first)
        for (int second = first + 1; second < HOURS && !found; ++second) {
            const auto replay = replay_schedule(problem, schedule(fixture, first, second));
            if (!replay.candidate.replay.strict_valid) continue;
            const auto& end = replay.candidate.end.physical;
            problem.required_end_tiles = {end_requirement(end.managed_tiles[0].state)};
            problem.end_shed = end.shed;
            problem.end_seeds = end.seeds;
            found = true;
        }
    assert(found);

    int comparisons = 0;
    for (int target = 0; target < 2; ++target) {
        if (target) {
            auto& end = *problem.required_end_tiles[0].exact_state;
            if (end.kind != ManagedTileKind::CROP &&
                !(animal_structure(end.kind) && end.animal >= 0)) break;
            ++end.age_days;  // A deliberately impossible endpoint.
        }
        const auto graphs = build_tile_graphs(problem);
        assert(graphs.size() == 1);
        for (int first = earliest; first < HOURS; ++first) {
            assert(!accepts(graphs[0], first, first));  // Only one worker.
            for (int second = first + 1; second < HOURS; ++second) {
                const auto replay = replay_schedule(problem, schedule(fixture, first, second));
                const bool expected = replay.candidate.replay.strict_valid &&
                    replay.requirements_satisfied && replay.invariants_satisfied;
                if (accepts(graphs[0], first, second) != expected) {
                    std::cerr << fixture.name << " target=" << target
                              << " hours=" << first << ',' << second << '\n';
                    std::abort();
                }
                ++comparisons;
            }
        }
    }
    return comparisons;
}

}  // namespace

int main() {
    std::vector<Fixture> fixtures;
    for (int crop = 0; crop < kag::N_CROPS; ++crop) {
        const auto& definition = kag::CROPS[crop];
        ManagedTileState state;
        state.kind = ManagedTileKind::CROP;
        state.crop = crop;
        state.age_days = definition.first_yield_day;
        state.stored_units = 1;
        fixtures.push_back({"crop_fertilize_water_" + std::to_string(crop), state,
                           {action(kag::OP_FERTILIZE), action(kag::OP_WATER)}, kag::FERTILIZER});
        const int loss_age = definition.ongoing ? definition.first_yield_day +
            (definition.max_yield - 1) * definition.interval + 1 : definition.max_yield_day + 1;
        state.age_days = loss_age;
        state.stored_units = std::min(3, definition.max_yield);
        for (int quantity = 1; quantity < state.stored_units; ++quantity)
            fixtures.push_back({"decay_water_harvest_" + std::to_string(crop) + '_' + std::to_string(quantity), state,
                               {action(kag::OP_WATER), action(kag::OP_HARVEST, crop, crop, quantity)}});
    }
    for (int animal = kag::GOOSE; animal <= kag::SHEEP; ++animal) {
        ManagedTileState state;
        state.kind = structure_for(animal);
        state.animal = animal;
        state.age_days = kag::ANIMALS[animal - kag::GOOSE].first_yield_day;
        state.stored_units = 2;
        state.pending_care_bonus = 2;
        state.fertilizer_available = true;
        const int product = kag::ANIMALS[animal - kag::GOOSE].product;
        fixtures.push_back({"animal_feed_harvest_" + std::to_string(animal), state,
                           {action(kag::OP_FEED), action(kag::OP_HARVEST, product, product, 2)}, kag::WHEAT});
        state.consecutive_dry_days = 1;
        fixtures.push_back({"animal_starve_care_collect_" + std::to_string(animal), state,
                           {action(kag::OP_CARE), action(kag::OP_COLLECT_FERTILIZER, -1, kag::FERTILIZER, 1)}});
        fixtures.push_back({"build_place_" + std::to_string(animal), {},
                           {action(animal == kag::GOOSE ? kag::OP_BUILD_COOP : kag::OP_BUILD_PASTURE),
                            action(kag::OP_PLACE, animal)}, animal});
    }
    int comparisons = 0;
    for (const auto& fixture : fixtures) comparisons += check_fixture(fixture);
    std::cout << fixtures.size() << " fixtures, " << comparisons
              << " exhaustive two-action timing comparisons passed\n";
}
