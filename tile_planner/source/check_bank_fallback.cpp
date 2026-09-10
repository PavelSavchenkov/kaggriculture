#include "schedule_bank.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 2) throw std::runtime_error("usage: check_bank_fallback source_course");
        const Course course(argv[1]); int repaired = 0, checked = 0;
        for (int d = 0; d < 30; ++d) for (const auto& tile : course.days[d].problem.tile_work) {
            if (tile.actions.empty() || tile.actions.front().op != kag::OP_PLANT) continue;
            const auto& source = course.days[d];
            if (source.problem.start.managed_tiles[tile.tile].state.kind != day_solver::ManagedTileKind::EMPTY) continue;
            auto day = source; day.problem.start.managed_tiles[tile.tile].state.kind = day_solver::ManagedTileKind::WEED;
            auto& work = *std::find_if(day.problem.tile_work.begin(), day.problem.tile_work.end(), [&](const auto& row) { return row.tile == tile.tile; });
            day_solver::TileWorkAction clear; clear.op = kag::OP_DIG; work.actions.insert(work.actions.begin(), clear);
            day_scheduler::prepare_problem(day.problem);
            ScheduleBank clean; clean.add(source.problem, source.physical, "source");
            const auto expected = clean.find(day, d);
            if (!expected.schedule) continue;
            // A stale exact-key entry must not mask a valid repair from the
            // simpler work group, and must never be returned as a witness.
            ScheduleBank stale; stale.add(day.problem, source.physical, "deliberately stale exact-key entry");
            stale.add(source.problem, source.physical, "source");
            const auto actual = stale.find(day, d); ++checked;
            if (!actual.schedule || actual.workers != expected.workers || actual.initial_witness != "source" || stale.weed_repairs != 1)
                throw std::runtime_error("stale exact group masked valid weed repair");
            const auto replay = day_solver::replay_schedule(*actual.problem, *actual.schedule);
            if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())
                throw std::runtime_error("invalid repaired bank witness");
            ++repaired;
        }
        if (!checked) throw std::runtime_error("test corpus contains no repairable examples");
        std::cout << "{\"checked\":" << checked << ",\"strict_valid_repairs\":" << repaired << "}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
