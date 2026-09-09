#include "planning_features.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: planning_tool manifest output");
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "id"; for (const auto& name : labor::planning_feature_names()) output << ',' << name;
        output << ",extraction_us\n";
        std::string id, path; int hours, count, completed = 0;
        while (input >> id >> path >> hours >> count) {
            if (count < 0 || count > 39) throw std::runtime_error("bad menu count");
            std::array<std::pair<int, int>, 39> slots{};
            for (int i = 0; i < count; ++i) if (!(input >> slots[i].first >> slots[i].second)) throw std::runtime_error("incomplete menu");
            const auto p = day_solver::load_problem_json(path);
            const auto menu = labor::planning_menu(p, hours, std::span(slots.data(), count));
            const auto start = std::chrono::steady_clock::now();
            const auto features = labor::extract_planning(p, menu);
            const auto us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
            auto changed = p; changed.worker_count = 40;
            std::erase_if(changed.market_plan, [](const auto& event) { return event.market_op == kag::M_HIRE; });
            if (features != labor::extract_planning(changed, menu)) throw std::runtime_error("planning features leaked source answer");
            const int n = changed.start.managed_tiles.size();
            std::reverse(changed.start.managed_tiles.begin(), changed.start.managed_tiles.end());
            for (auto& work : changed.tile_work) work.tile = n - 1 - work.tile;
            for (auto& end : changed.required_end_tiles) end.tile = n - 1 - end.tile;
            std::reverse(changed.tile_work.begin(), changed.tile_work.end());
            if (features != labor::extract_planning(changed, menu)) throw std::runtime_error("planning tile-order dependence");
            output << id << std::setprecision(9); for (float value : features) output << ',' << value;
            output << ',' << us << '\n'; ++completed;
        }
        if (!input.eof()) throw std::runtime_error("malformed planning manifest");
        std::cout << completed << " explicit-calendar feature inputs\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
