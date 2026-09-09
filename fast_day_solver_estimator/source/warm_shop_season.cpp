#include "warm_shop_season.hpp"
#include "actions.hpp"
#include "bounds.hpp"
#include "supply_bounds.hpp"
#include "original_labor.hpp"
#include "search_model.hpp"
#include <chrono>
#include <iomanip>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::filesystem::path output = argv[1];
    if (std::filesystem::exists(output)) return 2;
    std::filesystem::create_directories(output);
    const auto begin = std::chrono::steady_clock::now();
    const Season season(std::stoull(argv[2]));
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    std::ofstream csv(output / "DAYS.csv");
    csv << "day,workers,hire_cost,strict,requirements,invariants,errors,normalized_cost_prediction,original_geometry_cost,tasks\n" << std::setprecision(17);
    for (int day = 0; day < 30; ++day) {
        auto problem = season.days[day].problem;
        day_scheduler::prepare_problem(problem);
        auto actions = season.days[day].own;
        labor::offline::physical_orders(problem, actions);
        const auto replay = day_solver::replay_schedule(problem, actions);
        const auto directory = output / std::to_string(day);
        std::filesystem::create_directories(directory);
        day_solver::save_problem_json(problem, directory / "problem.json");
        labor::offline::save_actions(actions, (directory / "actions.txt").string());
        compositions::day_contract::save_actions(season.days[day].own, directory / "economic_actions.txt");
        const auto features = labor::extract(problem, day == 29 ? 23 : 24);
        const auto old = labor::original::estimate(labor::original::aggregate(problem));
        int price = 0;
        for (int h = 0; h < problem.worker_count - 1; ++h) price += kag::fib(h);
        csv << day << ',' << problem.worker_count << ',' << price << ',' << replay.candidate.replay.strict_valid
            << ',' << replay.requirements_satisfied << ',' << replay.invariants_satisfied << ',' << replay.errors.size()
            << ',' << (day == 29 ? -1 : labor::search_model::cost(features)) << ',' << old.cost << ',' << features[labor::tasks] << '\n';
    }
    std::ofstream report(output / "REPORT.json");
    report << "{\"seed\":" << argv[2] << ",\"source_action_hash\":" << season.hashes[0]
           << ",\"rival_action_hash\":" << season.hashes[1] << ",\"source_cash\":" << season.final_farms[0].money
           << ",\"rival_cash\":" << season.final_farms[1].money << ",\"extract_wall_seconds\":" << seconds
           << ",\"scope\":\"Configured shop-sequence scenario, 719 real transitions. Day29 has an offline empty final phase only; its normalized cost prediction is unsupported.\"}\n";
}
