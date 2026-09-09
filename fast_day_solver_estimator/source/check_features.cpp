#include "route_features.hpp"
#include <day_solver/io.hpp>
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]); std::string id, path; int count = 0;
    while (input >> id >> path) {
        auto p = day_solver::load_problem_json(path); const auto f = labor::extract(p);
        const auto route = labor::optimized_routes(p);
        auto changed = p; changed.worker_count = 40;
        std::erase_if(changed.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
        if (f != labor::extract(changed)) { std::cerr << "answer leakage " << id << '\n'; return 3; }
        if (route != labor::optimized_routes(changed)) { std::cerr << "route answer leakage " << id << '\n'; return 8; }
        const int n = p.start.managed_tiles.size();
        std::reverse(changed.start.managed_tiles.begin(), changed.start.managed_tiles.end());
        for (auto& work : changed.tile_work) work.tile = n - 1 - work.tile;
        for (auto& end : changed.required_end_tiles) end.tile = n - 1 - end.tile;
        if (f != labor::extract(changed)) { std::cerr << "tile-index dependence " << id << '\n'; return 4; }
        std::reverse(changed.tile_work.begin(), changed.tile_work.end());
        if (route != labor::optimized_routes(changed)) { std::cerr << "route indexing/order dependence " << id << '\n'; return 9; }
        const auto menu = labor::earliest_menu(p);
        const auto bound = labor::workforce_lower_bound(p, f, menu);
        if (bound < 1 || bound > 41) return 5;
        for (float v : f) if (!std::isfinite(v)) return 6;
        for (float v : route) if (!std::isfinite(v)) return 10;
        ++count;
    }
    if (!input.eof()) return 2;
    if (labor::hire_cost(1) != 0 || labor::hire_cost(6) != 12 || labor::hire_cost(11) != 143) return 7;
    std::cout << "answer isolation, tile-index invariance and finite features: " << count << " cases\n";
}
