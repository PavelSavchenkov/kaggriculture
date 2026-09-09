#include "route_features.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: route_tool manifest output active_hours repetitions");
        const int hours = std::stoi(argv[3]), repeats = std::stoi(argv[4]);
        if ((hours != 23 && hours != 24) || repeats < 1) throw std::runtime_error("bad horizon/repetitions");
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "id"; for (const auto* name : labor::route_feature_names) output << ',' << name;
        output << ",active_hours,route_us\n";
        std::string id, path; int count = 0;
        while (input >> id >> path) {
            const auto problem = day_solver::load_problem_json(path);
            labor::RouteFeatures values{};
            const auto started = std::chrono::steady_clock::now();
            for (int iteration = 0; iteration < repeats; ++iteration) {
                const auto current = labor::optimized_routes(problem, hours);
                if (iteration && current != values) throw std::runtime_error("nondeterministic route feature");
                values = current;
            }
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count() / repeats;
            output << id << std::setprecision(9); for (float value : values) output << ',' << value;
            output << ',' << hours << ',' << us << '\n'; ++count;
        }
        if (!input.eof()) throw std::runtime_error("bad manifest");
        std::cout << "Computed " << count << " route feature vectors\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
