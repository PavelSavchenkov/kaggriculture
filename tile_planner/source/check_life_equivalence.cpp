#include "life_search.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 2) throw std::runtime_error("usage: check_life_equivalence output_json");
        int checked = 0;
        std::mt19937_64 rng(999983);
        for (const auto& name : {"wheat12", "dairy", "mixed_animals", "geese", "crop_rotation", "expanding_mixed"}) {
            const auto lives = cold_case(name); const LifeEquivalence equivalence(lives);
            std::array<int, 30> land; for (int d = 0; d < 30; ++d) land[d] = cold_land_purchases(name, d);
            LifeEvaluator plain(lives, land, false), canonical(lives, land);
            auto assignment = assign_lives(lives, true, true);
            for (int iteration = 0; iteration < 96; ++iteration) {
                auto trial = assignment;
                if (iteration % 3) {
                    const auto& group = equivalence.groups[rng() % equivalence.groups.size()];
                    std::swap(trial[group[rng() % group.size()]], trial[group[rng() % group.size()]]);
                } else {
                    const int first = rng() % lives.size(), second = rng() % lives.size();
                    std::swap(trial[first], trial[second]);
                }
                if (!legal_lives(lives, trial)) continue;
                const auto a = plain(trial), b = canonical(trial);
                if (a.day_cost != b.day_cost || a.day_workers != b.day_workers || a.lower != b.lower || a.rejected != b.rejected)
                    throw std::runtime_error("identical-life canonicalization changed a prediction");
                assignment = std::move(trial); ++checked;
            }
        }
        std::ofstream output(argv[1]); output << "{\"checked\":" << checked << ",\"bit_exact\":true,\"cases\":6}\n";
        std::cout << checked << " legal assignments: exact prediction agreement\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
