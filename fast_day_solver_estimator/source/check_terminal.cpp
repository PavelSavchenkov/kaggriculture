#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <iostream>

using namespace day_solver;
using namespace kag;

bool valid(const DayProblem& problem, const std::array<Action, 24>& schedule) {
    const auto replay = replay_schedule(problem, schedule);
    return replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
}

int main() {
    try {
        DayProblem p; ManagedTileState crop; crop.kind = ManagedTileKind::CROP; crop.crop = WHEAT;
        crop.stored_units = 1; crop.consecutive_dry_days = 1;
        p.start.managed_tiles.push_back({4, 4, crop});
        auto end_state = crop; end_state.age_days = 1; end_state.consecutive_dry_days = 0;
        EndTileRequirement end; end.tile = 0; end.exact_state = end_state; p.required_end_tiles.push_back(end);
        TileWorkAction water; water.op = OP_WATER; p.tile_work.push_back({0, {water}});
        day_scheduler::prepare_problem(p);
        std::array<Action, 24> schedule;
        for (auto& action : schedule) action.finalize();
        schedule[23].units[0].op = OP_WATER; schedule[23].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("ordinary last-phase control is invalid");
        schedule[23] = Action{}; schedule[23].finalize();
        if (valid(p, schedule)) throw std::runtime_error("missing terminal work incorrectly accepted");
        schedule[22].units[0].op = OP_WATER; schedule[22].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("valid23-phase water rejected");
        crop.age_days = 2; crop.consecutive_dry_days = 0;
        p.start.managed_tiles[0].state = crop; p.required_end_tiles[0].exact_state = ManagedTileState{};
        TileWorkAction harvest; harvest.op = OP_HARVEST; harvest.arg = harvest.output_item = WHEAT; harvest.output_quantity = 1;
        p.tile_work[0].actions = {harvest}; p.end_shed[WHEAT] = 1;
        day_scheduler::prepare_problem(p);
        schedule[22].units[0].op = OP_HARVEST; schedule[22].finalize();
        schedule[23].units[0].op = OP_DROP; schedule[23].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("ordinary last-drop control invalid");
        schedule[23] = Action{}; schedule[23].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("terminal remaining-goods total rejected");
        const auto f = labor::extract(p, 23);
        if (labor::workforce_lower_bound(p, f, labor::earliest_menu(p, 23), 23) > 1)
            throw std::runtime_error("terminal bound contradicts the known one-worker schedule");
        std::cout << "Terminal controls passed: missing required phase23 work rejected; work by22 accepted; remaining cargo does not require a real overnight transfer.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
