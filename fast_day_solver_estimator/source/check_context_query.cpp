#include "context_query_features.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: check_context_query feature_manifest output");
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "id,workers";
        for (int i = 0; i < labor::context_query_count; ++i) output << ",q" << i;
        output << '\n' << std::setprecision(17);
        std::string id; int workers, completed = 0;
        while (input >> id >> workers) {
            labor::ContextFeatures f{};
            for (float& value : f) if (!(input >> value)) throw std::runtime_error("incomplete context row");
            const auto q = labor::context_query_features(f, workers);
            auto changed = f;
            // Unselected slots and full-menu summaries may vary independently.
            for (int i = workers - 1; i < 39; ++i) changed[labor::planning_birth_base + i] = int(f[labor::planning_hours]);
            changed[labor::planning_slots] = workers - 1;
            for (int i : {int(labor::planning_lower), int(labor::planning_supply_base), labor::planning_supply_base + 3,
                          labor::planning_supply_base + 4, int(labor::planning_release_base), labor::planning_release_base + 5,
                          labor::planning_release_base + 6}) changed[i] += 13;
            if (q != labor::context_query_features(changed, workers)) throw std::runtime_error("unselected choices changed the query");
            std::reverse(changed.begin() + labor::planning_birth_base, changed.begin() + labor::planning_birth_base + workers - 1);
            if (q != labor::context_query_features(changed, workers)) throw std::runtime_error("selection order changed the query");
            changed.back() = workers;
            if (q != labor::context_query_features(changed, workers)) throw std::runtime_error("mandatory designation changed identical cold call");
            output << id << ',' << workers; for (double value : q) output << ',' << value;
            output << '\n'; ++completed;
        }
        if (!input.eof()) throw std::runtime_error("malformed context manifest");
        std::cout << completed << " query feature and context-isolation checks\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
