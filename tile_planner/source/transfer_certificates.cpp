#include "certify.hpp"
#include "timeline.hpp"

struct BankDay { day_solver::DayProblem problem; placement::Schedule schedule; std::string source; };

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 5) throw std::runtime_error("usage: transfer_certificates course layout new_output bank_directories...");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]); const auto timeline = read_timeline(argv[2]); const fs::path output(argv[3]);
        if (!legal_timeline(course, timeline) || fs::exists(output)) throw std::runtime_error("invalid timeline or output exists");
        fs::create_directories(output);
        std::array<std::vector<BankDay>, 30> bank;
        for (int arg = 4; arg < argc; ++arg) for (int d = 0; d < 30; ++d) {
            const auto folder = fs::path(argv[arg]) / day_name(d);
            if (!fs::exists(folder / "problem.json")) continue;
            bank[d].push_back({day_solver::load_problem_json(folder / "problem.json"),
                labor::offline::read_actions((folder / "physical.actions.txt").string()), fs::path(argv[arg]).filename().string()});
        }
        std::ofstream report(output / "days.csv"); report << "day,certified,workers,bill,replay_checks,source\n";
        int valid = 0, checks = 0; int64_t bill = 0;
        for (int d = 0; d < 30; ++d) {
            const auto& layout = timeline[d];
            const auto base = course.problem(d, layout); const auto& source = course.days[d];
            auto best = certify_day(course, d, layout, 0, 0);
            std::string chosen = best.initial_witness;
            int local_checks = 0;
            for (const auto& entry : bank[d]) {
                if (entry.problem.worker_count >= best.workers) continue;
                auto problem = base;
                std::erase_if(problem.market_plan, [](const auto& event) { return event.market_op == kag::M_HIRE; });
                bool collision = false;
                for (const auto& event : entry.problem.market_plan) if (event.market_op == kag::M_HIRE) {
                    const auto& original = source.executable[event.hour].orders[event.order_index];
                    collision |= original.op != kag::M_NONE && original.op != kag::M_HIRE;
                    problem.market_plan.push_back(event);
                }
                if (collision) continue;
                problem.worker_count = entry.problem.worker_count;
                std::sort(problem.market_plan.begin(), problem.market_plan.end(), [](const auto& a, const auto& b) {
                    return std::pair{a.hour, a.order_index} < std::pair{b.hour, b.order_index};
                });
                day_scheduler::prepare_problem(problem);
                if (d == 29) labor::offline::require_terminal_work(problem);
                auto actions = entry.schedule; labor::offline::physical_orders(problem, actions);
                const auto replay = day_solver::replay_schedule(problem, actions); ++local_checks;
                if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty()) continue;
                best.schedule = actions; best.problem = std::move(problem); best.workers = entry.problem.worker_count; chosen = entry.source;
            }
            checks += local_checks;
            const bool ok = best.schedule.has_value(); valid += ok;
            if (ok) {
                const auto folder = output / day_name(d); fs::create_directories(folder);
                day_solver::save_problem_json(*best.problem, folder / "problem.json");
                labor::offline::save_actions(*best.schedule, (folder / "physical.actions.txt").string());
                labor::offline::save_actions(executable(source, *best.problem, *best.schedule), (folder / "executable.actions.txt").string());
                bill += labor::hire_cost(best.workers);
            }
            report << d << ',' << ok << ',' << best.workers << ',' << (ok ? labor::hire_cost(best.workers) : -1) << ',' << local_checks << ',' << chosen << '\n';
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"complete\":" << (valid == 30 ? "true" : "false") << ",\"certified_days\":" << valid << ",\"certified_partial_bill\":" << bill
                << ",\"replay_checks\":" << checks << ",\"seconds\":" << seconds << ",\"solver_queries\":0,\"full_engine_verified\":false}\n";
        std::cout << valid << "/30 days; bill " << bill << "; seconds " << seconds << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
