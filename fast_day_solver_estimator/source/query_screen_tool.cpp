#include "actions.hpp"
#include "query_screen.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: query_screen_tool features|audit manifest output active_hours");
        const std::string mode = argv[1]; const int hours = std::stoi(argv[4]);
        if (mode != "features" && mode != "audit") throw std::runtime_error("invalid mode");
        std::ifstream input(argv[2]); std::ofstream output(argv[3]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "id,workers,capacity_bound,supply_bound,release_bound,reasons,screen_us";
        if (mode == "audit") output << ",strict";
        output << '\n';
        std::string id, path, witness; int count = 0, valid_count = 0, screened = 0;
        while (input >> id >> path) {
            const auto p = day_solver::load_problem_json(path);
            const auto start = std::chrono::steady_clock::now();
            const auto result = labor::screen_query(p, hours);
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
            output << id << ',' << result.requested_workers << ',' << result.capacity_bound << ',' << result.supply_bound << ',' << result.release_bound << ',' << result.reasons << ',' << std::setprecision(9) << us;
            if (mode == "audit") {
                if (!(input >> witness)) throw std::runtime_error("missing witness");
                auto schedule = labor::offline::read_actions(witness); labor::offline::physical_orders(p, schedule);
                const auto replay = day_solver::replay_schedule(p, schedule);
                bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
                if (hours == 23) valid &= !schedule[23].n_orders && std::all_of(schedule[23].units, schedule[23].units + schedule[23].n_units, [](const auto& a) { return a.op == kag::OP_PASS; });
                if (valid && result.reasons) throw std::runtime_error("query screen contradicts valid fixed-calendar witness: " + id);
                output << ',' << valid; valid_count += valid;
            }
            output << '\n'; ++count; screened += bool(result.reasons);
        }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::cout << "Screened " << count << " exact-calendar queries; " << screened << " rejected; " << valid_count << " verified witnesses\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
