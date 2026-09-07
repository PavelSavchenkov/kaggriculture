#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

#include "problem_json.hpp"
#include "problem_validation.hpp"
#include "replay.hpp"

using namespace day_solver;

namespace {

constexpr InventoryCount LIMIT = std::numeric_limits<int32_t>::max();

DayProblem empty() {
    DayProblem problem;
    problem.start.shed_capacity = std::numeric_limits<int16_t>::max();
    problem.start.managed_tiles.push_back({4, 4, {}});
    problem.required_end_tiles.push_back({0, ManagedTileKind::EMPTY, NO_SUBJECT,
                                          ManagedTileState{}});
    return problem;
}

using Schedule = std::array<kag::Action, HOURS>;

void buy(DayProblem& problem, Schedule& schedule, int hour, int slot,
         int op, int item, int quantity) {
    problem.market_plan.push_back({static_cast<int8_t>(hour), static_cast<int8_t>(slot),
        static_cast<uint8_t>(op), static_cast<int16_t>(item), quantity, 0});
    schedule[hour].n_orders = std::max(schedule[hour].n_orders, slot + 1);
    schedule[hour].orders[slot] = {static_cast<uint8_t>(op), static_cast<uint8_t>(item), quantity};
}

void withdraw(DayProblem& problem, int hour, InventoryCount quantity) {
    for (; hour < HOURS; ++hour) problem.shed_availability[hour][kag::WHEAT] = quantity;
}

ReplayResult replay(const DayProblem& problem, const Schedule& schedule) {
    // Exercise the public JSON path, including exact terminal bounds.
    const auto parsed = parse_problem_json(serialize_problem_json(problem));
    assert(validate_problem(parsed).empty());
    assert(parsed.start.shed == problem.start.shed);
    assert(parsed.start.seeds == problem.start.seeds);
    return replay_schedule(parsed, schedule);
}

void accepted(const ReplayResult& result) {
    assert(result.candidate.replay.strict_valid);
    assert(result.requirements_satisfied && result.invariants_satisfied);
}

}  // namespace

int main() {
    {
        // Native callers must receive the same exact end checks as JSON callers.
        auto problem = empty();
        Schedule schedule{};
        problem.start.shed[kag::WHEAT] = 1;
        auto result = replay_schedule(problem, schedule);
        assert(result.candidate.replay.strict_valid && !result.requirements_satisfied);
        problem.end_shed[kag::WHEAT] = 1;
        accepted(replay_schedule(problem, schedule));
        problem.end_seeds[kag::WHEAT] = 1;
        assert(!replay_schedule(problem, schedule).requirements_satisfied);
    }
    {
        auto problem = empty();
        Schedule schedule{};
        buy(problem, schedule, 0, 0, kag::M_BUY_PRODUCT, kag::WHEAT, 65536);
        auto result = replay(problem, schedule);
        assert(!result.requirements_satisfied);
        assert(result.candidate.end.physical.shed[kag::WHEAT] == 65536);
        problem.end_shed[kag::WHEAT] = 65536;
        accepted(replay(problem, schedule));
    }
    for (InventoryCount initial : {InventoryCount{32767}, LIMIT}) {
        auto problem = empty();
        Schedule schedule{};
        problem.start.shed[kag::WHEAT] = initial;
        problem.end_shed[kag::WHEAT] = initial;
        buy(problem, schedule, 0, 0, kag::M_BUY_PRODUCT, kag::WHEAT, 1);
        withdraw(problem, 1, 1);
        auto result = replay(problem, schedule);
        accepted(result);
        assert(result.candidate.hours[0].shed_after_market[kag::WHEAT] == initial + 1);
    }
    {
        auto problem = empty();
        Schedule schedule{};
        problem.start.shed[kag::WHEAT] = LIMIT;
        problem.end_shed[kag::WHEAT] = LIMIT;
        buy(problem, schedule, 0, 0, kag::M_BUY_PRODUCT, kag::WHEAT, 1);
        withdraw(problem, 1, 1);
        schedule[1].units[0] = {kag::OP_PICKUP, kag::WHEAT, static_cast<int32_t>(LIMIT)};
        accepted(replay(problem, schedule));  // Automatic final cargo transfer.
    }
    for (bool selective : {false, true}) {
        auto problem = empty();
        Schedule schedule{};
        problem.start.shed[kag::WHEAT] = LIMIT;
        problem.end_shed[kag::WHEAT] = LIMIT;
        buy(problem, schedule, 0, 0, kag::M_BUY_PRODUCT, kag::WHEAT, LIMIT);
        schedule[0].units[0] = {kag::OP_PICKUP, kag::WHEAT, static_cast<int32_t>(LIMIT)};
        schedule[1].units[0] = schedule[0].units[0];
        schedule[2].units[0] = {static_cast<uint8_t>(selective ? kag::OP_PLACE : kag::OP_DROP),
                                kag::WHEAT, static_cast<int32_t>(LIMIT)};
        withdraw(problem, 2, LIMIT);
        accepted(replay(problem, schedule));  // Carried stock reaches 2*LIMIT.
    }
    {
        auto problem = empty();
        Schedule schedule{};
        problem.start.seeds[kag::WHEAT] = LIMIT;
        problem.end_seeds[kag::WHEAT] = LIMIT;
        buy(problem, schedule, 0, 0, kag::M_BUY_SEED, kag::WHEAT, 1);
        problem.tile_work = {{0, {{kag::OP_PLANT, kag::WHEAT, 1, NO_SUBJECT, 0},
                                  {kag::OP_WATER, NO_SUBJECT, 1, NO_SUBJECT, 0}}}};
        ManagedTileState end;
        end.kind = ManagedTileKind::CROP;
        end.crop = kag::WHEAT;
        end.age_days = 1;
        end.stored_units = 1;
        problem.required_end_tiles = {{0, end.kind, end.crop, end}};
        schedule[1].units[0] = {kag::OP_PLANT, kag::WHEAT, 1};
        schedule[2].units[0] = {kag::OP_WATER, 0, 1};
        accepted(replay(problem, schedule));
    }
    {
        auto problem = empty();
        Schedule schedule{};
        for (int hour = 0; hour < HOURS; ++hour)
            for (int slot = 0; slot < 10; ++slot)
                buy(problem, schedule, hour, slot, kag::M_BUY_PRODUCT, kag::WHEAT, LIMIT);
        const InventoryCount total = LIMIT * HOURS * 10;
        const auto result = replay(problem, schedule);
        assert(result.candidate.replay.strict_valid && !result.requirements_satisfied);
        assert(result.candidate.end.physical.shed[kag::WHEAT] == total);
        assert(result.candidate.costs.purchased_units == static_cast<uint64_t>(total));
        assert(result.candidate.hours.back().headroom_after_market == problem.start.shed_capacity - total);
    }
    {
        auto problem = empty();
        problem.start.shed[kag::WHEAT] = LIMIT + 1;
        assert(!validate_problem(problem).empty());
        bool rejected = false;
        try {
            (void)parse_problem_json(serialize_problem_json(problem));
        } catch (const std::exception&) {
            rejected = true;
        }
        assert(rejected);
    }
    std::cout << "Replay inventory boundary checks passed\n";
}
