#include "fast_day_solver_estimator/estimator.hpp"
#include <day_solver/io.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 4) throw std::runtime_error("usage: estimate_day problem.json [active_hours=24] [menu.txt]");
        const auto problem = day_solver::load_problem_json(argv[1]);
        const int hours = argc >= 3 ? std::stoi(argv[2]) : 24;
        labor::PlanningMenu menu;
        if (argc < 4) menu = fast_day_solver_estimator::earliest_hiring_menu(problem, hours);
        if (argc == 4) {
            std::ifstream file(argv[3]); int fixed_count, optional_count;
            if (!(file >> fixed_count >> optional_count) || fixed_count < 0 || optional_count < 0 || fixed_count + optional_count > 39)
                throw std::runtime_error("invalid menu counts");
            std::array<std::pair<int, int>, 39> fixed{}, optional{};
            for (int i = 0; i < fixed_count; ++i) if (!(file >> fixed[i].first >> fixed[i].second)) throw std::runtime_error("incomplete fixed menu");
            for (int i = 0; i < optional_count; ++i) if (!(file >> optional[i].first >> optional[i].second)) throw std::runtime_error("incomplete optional menu");
            std::string extra; if (file >> extra) throw std::runtime_error("trailing menu fields");
            menu = labor::fixed_planning_menu(problem, hours, std::span(fixed.data(), fixed_count), std::span(optional.data(), optional_count));
        }
        const auto r = fast_day_solver_estimator::estimate_day(problem, menu);
        std::cout << std::setprecision(17) << std::boolalpha;
        std::cout << "{\"active_hours\":" << hours << ",\"analytically_rejected\":" << r.analytically_rejected
                  << ",\"minimum_workers_lower_bound\":" << r.lower << ",\"estimated_workers\":";
        if (r.analytically_rejected) std::cout << "null,\"estimated_hire_cost\":null";
        else std::cout << r.workers << ",\"estimated_hire_cost\":" << r.cost;
        std::cout << ",\"uses_direct_cost\":" << r.uses_direct_cost << ",\"weak_probe\":" << r.weak_probe
                  << ",\"low_peak\":" << r.low_peak << ",\"complete_curve\":" << r.complete_curve
                  << ",\"probe_workers\":" << r.probe_workers << ",\"probe_probability\":";
        if (r.probe_probability < 0) std::cout << "null"; else std::cout << r.probe_probability;
        std::cout << ",\"query_evaluations\":" << r.query_evaluations << ",\"certificate\":false}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
