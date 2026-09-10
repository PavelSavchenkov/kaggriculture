#include "population.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 5) throw std::runtime_error("usage: search_portfolio course new_output seconds_per_search seed donor_courses...");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]); const fs::path output(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        const double seconds = std::stod(argv[3]); const uint64_t seed = std::stoull(argv[4]);
        Evaluator evaluate(course); auto candidates = rules(course, evaluate);
        for (int arg = 5; arg < argc; ++arg) {
            const Course donor(argv[arg]);
            for (int variant = 0; variant < 2; ++variant) {
                const auto layout = donor_layout(course, donor, variant ? 1. : .2);
                if (std::any_of(candidates.begin(), candidates.end(), [&](const auto& old) { return old.layout == layout; })) continue;
                candidates.push_back({"donor_" + std::to_string(arg - 5) + "_" + std::to_string(variant), layout, evaluate(layout)});
            }
        }
        const auto seeds = candidates;
        const auto best_seed = *std::min_element(seeds.begin(), seeds.end(), [](const auto& a, const auto& b) { return a.score.cost < b.score.cost; });
        candidates.push_back(swap_search(course, evaluate, best_seed, seconds, seed, false));
        candidates.push_back(population_search(course, evaluate, seeds, seconds, seed + 1, false));
        candidates.push_back(population_search(course, evaluate, seeds, seconds, seed + 2, true));
        std::ofstream report(output / "candidates.csv");
        report << "method,predicted_cost,predicted_workers,rejected_days,weak_days,changed_cells\n" << std::setprecision(12);
        for (const auto& candidate : candidates) {
            std::ofstream file(output / (candidate.method + ".layout.txt")); int changed = 0;
            for (int i = 0; i < 100; ++i) { file << int(candidate.layout[i]) << ' '; changed += candidate.layout[i] != i; }
            file << '\n';
            report << candidate.method << ',' << candidate.score.cost << ',' << candidate.score.workers << ',' << candidate.score.rejected << ',' << candidate.score.weak << ',' << changed << '\n';
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"seconds\":" << elapsed << ",\"layouts\":" << evaluate.queries << ",\"cache_hits\":" << evaluate.hits
                << ",\"day_queries\":" << evaluate.day_queries << ",\"day_cache_hits\":" << evaluate.day_hits << ",\"evictions\":" << evaluate.evictions << "}\n";
        std::cout << evaluate.queries << " layouts; " << elapsed << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
