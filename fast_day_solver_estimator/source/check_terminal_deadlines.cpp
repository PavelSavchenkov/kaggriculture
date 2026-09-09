#include "actions.hpp"
#include "terminal_deadlines.hpp"
#include <day_solver/io.hpp>
#include <iostream>

bool valid(const day_solver::DayProblem& p, const std::array<kag::Action, 24>& schedule) {
    const auto replay = day_solver::replay_schedule(p, schedule);
    return replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: check_terminal_deadlines terminal_witness_manifest");
        using namespace day_solver; using namespace kag;
        DayProblem p; ManagedTileState crop; crop.kind = ManagedTileKind::CROP; crop.crop = WHEAT;
        crop.stored_units = 1; crop.consecutive_dry_days = 1;
        p.start.managed_tiles.push_back({4, 4, crop});
        auto end = crop; end.age_days = 1; end.consecutive_dry_days = 0;
        EndTileRequirement requirement; requirement.tile = 0; requirement.exact_state = end;
        p.required_end_tiles.push_back(requirement);
        TileWorkAction water; water.op = OP_WATER; p.tile_work.push_back({0, {water}});
        day_scheduler::prepare_problem(p);
        std::array<Action, 24> schedule; for (auto& action : schedule) action.finalize();
        schedule[23].units[0].op = OP_WATER; schedule[23].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("ordinary control is invalid");
        labor::offline::require_terminal_work(p);
        if (valid(p, schedule)) throw std::runtime_error("terminal bound permits required work in phase 23");
        schedule[23] = Action{}; schedule[23].finalize(); schedule[22].units[0].op = OP_WATER; schedule[22].finalize();
        if (!valid(p, schedule)) throw std::runtime_error("terminal bound rejects phase 22 work");
        day_scheduler::Options options; options.seconds = 1; options.fallback_workers = 1;
        const auto solved = day_scheduler::solve(p, options);
        if (!solved.schedule || !valid(p, *solved.schedule)) throw std::runtime_error("deadline-constrained simple solve failed");
        std::ifstream input(argv[1]); if (!input) throw std::runtime_error("missing manifest");
        std::string id, path, witness; int checked = 0;
        while (input >> id >> path >> witness) {
            auto problem = load_problem_json(path); auto actions = labor::offline::read_actions(witness);
            labor::offline::physical_orders(problem, actions);
            if (!valid(problem, actions)) throw std::runtime_error("unverified input witness: " + id);
            const auto& last = actions[23];
            if (last.n_orders || !std::all_of(last.units, last.units + last.n_units, [](const auto& a) { return a.op == OP_PASS; }))
                throw std::runtime_error("witness uses virtual phase: " + id);
            labor::offline::require_terminal_work(problem);
            if (!valid(problem, actions)) throw std::runtime_error("new deadlines reject valid terminal witness: " + id);
            ++checked;
        }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::cout << checked << " strict terminal witnesses preserved; phase-23 rejection, phase-22 acceptance and constrained solver controls pass\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
