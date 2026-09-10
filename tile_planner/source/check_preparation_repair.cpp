#include "schedule_bank.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 2 || fs::exists(argv[1])) throw std::runtime_error("usage: check_preparation_repair new_report");
        Day empty;
        empty.problem.start.managed_tiles.push_back({4, 4, {}});
        day_solver::EndTileRequirement end; end.tile = 0; end.exact_state = day_solver::ManagedTileState{};
        empty.problem.required_end_tiles.push_back(end);
        for (auto* schedule : {&empty.physical, &empty.executable}) for (auto& a : *schedule) { a.n_units = 1; a.finalize(); }
        day_scheduler::prepare_problem(empty.problem); empty.menu = legal_menu(empty, 24);
        auto check = [](const DayProblem& p, const Schedule& s) {
            const auto r = day_solver::replay_schedule(p, s);
            return r.candidate.replay.strict_valid && r.requirements_satisfied && r.invariants_satisfied && r.errors.empty();
        };
        if (!check(empty.problem, empty.physical)) throw std::runtime_error("invalid empty fixture");
        ScheduleBank base; base.add(empty.problem, empty.physical, "empty");
        int checked = 0;
        for (int d : {0, 29}) for (bool weed : {false, true}) {
            auto target = empty;
            if (weed) target.problem.start.managed_tiles[0].state.kind = day_solver::ManagedTileKind::WEED;
            day_solver::TileWork work; work.tile = 0;
            if (weed) { day_solver::TileWorkAction dig; dig.op = kag::OP_DIG; work.actions.push_back(dig); }
            day_solver::TileWorkAction build; build.op = kag::OP_BUILD_COOP; work.actions.push_back(build);
            target.problem.tile_work.push_back(work);
            target.problem.required_end_tiles[0].exact_state->kind = day_solver::ManagedTileKind::COOP;
            day_scheduler::prepare_problem(target.problem); if (d == 29) labor::offline::require_terminal_work(target.problem);
            const auto repaired = base.find(target, d, 40, true);
            if (!repaired.schedule || !check(*repaired.problem, *repaired.schedule)) throw std::runtime_error("missing strict preparation insertion");
            ++checked;
            auto impossible = target; impossible.problem.end_shed[kag::MILK] = 1;
            if (base.find(impossible, d, 40, true).schedule) throw std::runtime_error("preparation repair invented output");
            ++checked;
            ScheduleBank reverse; reverse.add(*repaired.problem, *repaired.schedule, "prepared");
            const auto removed = reverse.find(empty, d, 40, true);
            if (!removed.schedule || !check(*removed.problem, *removed.schedule)) throw std::runtime_error("missing strict preparation removal");
            ++checked;
        }
        const auto identity = base.find(empty, 0, 40, true);
        if (!identity.schedule || !check(*identity.problem, *identity.schedule)) throw std::runtime_error("preparation repair broke identity");
        ++checked;
        std::ofstream report(argv[1]); report << "{\"checks\":" << checked << ",\"passed\":true,\"scope\":\"insert/remove preparation, terminal deadline, impossible output and identity\"}\n";
        std::cout << checked << " strict preparation checks passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
