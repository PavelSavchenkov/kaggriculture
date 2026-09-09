#include "actions.hpp"
#include "release_bounds.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: release_tool features|audit manifest output active_hours");
        const std::string mode = argv[1]; const int hours = std::stoi(argv[4]);
        if (mode != "features" && mode != "audit") throw std::runtime_error("unknown mode");
        std::ifstream input(argv[2]); std::ofstream output(argv[3]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "id,active_hours,release_lower_bound,seed_missing,land_missing,seed_late_actions,land_late_actions,release_tight_hour,release_tight_radius,release_pressure,release_us";
        if (mode == "audit") output << ",strict,workers";
        output << '\n';
        std::string id, path, witness; int count = 0, valid_count = 0;
        while (input >> id >> path) {
            const auto p = day_solver::load_problem_json(path);
            const auto started = std::chrono::steady_clock::now();
            const auto b = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count();
            auto changed = p; changed.worker_count = 40;
            std::erase_if(changed.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
            if (b != labor::release_bound(changed, labor::earliest_menu(changed, hours), hours)) throw std::runtime_error("release answer leakage");
            const int n = changed.start.managed_tiles.size();
            std::reverse(changed.start.managed_tiles.begin(), changed.start.managed_tiles.end());
            for (auto& work : changed.tile_work) work.tile = n - 1 - work.tile;
            std::reverse(changed.tile_work.begin(), changed.tile_work.end());
            if (b != labor::release_bound(changed, labor::earliest_menu(changed, hours), hours)) throw std::runtime_error("release tile-order dependence");
            output << id << ',' << hours << ',' << b.workers << ',' << b.seed_missing << ',' << b.land_missing << ',' << b.seed_late_actions << ',' << b.land_late_actions << ',' << b.tight_release << ',' << b.tight_radius << ',' << std::setprecision(9) << b.pressure << ',' << us;
            if (mode == "audit") {
                if (!(input >> witness)) throw std::runtime_error("missing witness");
                auto schedule = labor::offline::read_actions(witness); labor::offline::physical_orders(p, schedule);
                const auto replay = day_solver::replay_schedule(p, schedule);
                bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
                if (hours == 23) valid &= !schedule[23].n_orders && std::all_of(schedule[23].units, schedule[23].units + schedule[23].n_units, [](const auto& a) { return a.op == kag::OP_PASS; });
                if (valid && (b.workers > p.worker_count || b.seed_missing || b.land_missing)) throw std::runtime_error("release bound contradicts witness: " + id);
                output << ',' << valid << ',' << p.worker_count; valid_count += valid;
            }
            output << '\n'; ++count;
        }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::cout << "Release " << mode << ": " << count << " inputs, " << valid_count << " verified witnesses\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
