#include "solve_contract.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 6 && argc != 7) throw std::runtime_error("usage: solve_point input_day_folder day workers seconds new_output [dawn_order_delay]");
        const fs::path input(argv[1]), output(argv[5]); const int d = std::stoi(argv[2]), workers = std::stoi(argv[3]);
        if (fs::exists(output) || d < 0 || d >= 30) throw std::runtime_error("invalid point input or output exists");
        fs::create_directories(output);
        Day day; day.problem = day_solver::load_problem_json(input / "problem.json");
        day.executable = labor::offline::read_actions((input / "executable.actions.txt").string());
        const int delay = argc == 7 ? std::stoi(argv[6]) : 0;
        if (delay < 0 || delay > 2) throw std::runtime_error("invalid dawn order delay");
        if (delay) {
            for (auto& action : day.executable) for (auto& order : action.orders) if (order.op == kag::M_HIRE) order = {};
            for (int slot = 0; slot < 10; ++slot) {
                if (day.executable[delay].orders[slot].op != kag::M_NONE) throw std::runtime_error("delayed dawn order collision");
                day.executable[delay].orders[slot] = day.executable[0].orders[slot];
                day.executable[0].orders[slot] = {};
            }
            day.executable[delay].n_orders = day.executable[0].n_orders;
            day.executable[0].n_orders = 0;
            for (auto& event : day.problem.market_plan) if (event.hour == 0 && event.market_op != kag::M_HIRE) event.hour = delay;
            for (int h = 0; h < delay; ++h) day.problem.shed_availability[h].fill(0);
        }
        day.menu = legal_menu(day, d == 29 ? 23 : 24);
        const auto p = workforce(day.problem, day.menu, workers, d);
        day_scheduler::Options options; options.seconds = std::stod(argv[4]); options.fallback_workers = 1;
        const auto began = std::chrono::steady_clock::now(); const auto result = day_scheduler::solve(p, options);
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        day_solver::save_problem_json(p, output / "problem.json");
        if (result.schedule) {
            labor::offline::save_actions(*result.schedule, output / "physical.actions.txt");
            labor::offline::save_actions(executable(day, p, *result.schedule), output / "executable.actions.txt");
        }
        std::ofstream summary(output / "SUMMARY.json"); summary << "{\"day\":" << d << ",\"workers\":" << workers << ",\"seconds\":" << seconds
            << ",\"dawn_order_delay\":" << delay << ",\"status\":\"" << (result.schedule ? "feasible" : "unknown") << "\",\"bill\":" << labor::hire_cost(workers) << "}\n";
        std::cout << (result.schedule ? "feasible" : "unknown") << " at " << workers << " workers; " << seconds << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
