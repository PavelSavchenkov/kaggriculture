#include "actions.hpp"
#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: horizon_tool features|bounds|audit manifest output active_hours");
        const std::string mode = argv[1]; const int hours = std::stoi(argv[4]);
        if (hours != 23 && hours != 24) throw std::runtime_error("unsupported horizon");
        std::ifstream input(argv[2]); std::ofstream output(argv[3]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        if (mode == "features") {
            output << "id"; for (const auto& name : labor::feature_names()) output << ',' << name;
            output << ",active_hours,extraction_us\n";
        } else if (mode == "bounds") output << "id,lower_bound,deadline_missing_quantity\n";
        else if (mode == "audit") output << "id,strict,requirements,invariants,errors,lower_bound,workers,terminal_phase_empty\n";
        else throw std::runtime_error("unknown mode");
        std::string id, path, witness; int count = 0;
        while (input >> id >> path) {
            const auto p = day_solver::load_problem_json(path);
            const auto started = std::chrono::steady_clock::now();
            const auto f = labor::extract(p, hours);
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count();
            const int bound = labor::workforce_lower_bound(p, f, labor::earliest_menu(p, hours), hours);
            if (mode == "features") {
                output << id << std::setprecision(9); for (float v : f) output << ',' << v;
                output << ',' << hours << ',' << us << '\n';
            } else if (mode == "bounds") output << id << ',' << bound << ',' << f[labor::deadline_missing_quantity] << '\n';
            else {
                if (!(input >> witness)) throw std::runtime_error("missing witness");
                auto schedule = labor::offline::read_actions(witness); labor::offline::physical_orders(p, schedule);
                const auto& last = schedule[23];
                const bool empty = !last.n_orders && std::all_of(last.units, last.units + last.n_units, [](const auto& a) { return a.op == kag::OP_PASS; });
                const auto replay = day_solver::replay_schedule(p, schedule);
                const bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty() && (hours == 24 || empty);
                if (valid && (bound > p.worker_count || f[labor::deadline_missing_quantity] > 0)) throw std::runtime_error("necessary condition contradicts a valid witness");
                output << id << ',' << replay.candidate.replay.strict_valid << ',' << replay.requirements_satisfied << ',' << replay.invariants_satisfied
                       << ',' << replay.errors.size() << ',' << bound << ',' << p.worker_count << ',' << empty << '\n';
            }
            ++count;
        }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::cout << "Completed " << count << ' ' << mode << " records with " << hours << " active phases\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
