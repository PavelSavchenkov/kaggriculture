#include "population.hpp"
#include "timeline_search.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 5) throw std::runtime_error("usage: search_constrained course new_output seconds_per_search seed donor_courses...");
        const Course course(argv[1]); const fs::path output(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        const double seconds = std::stod(argv[3]); const uint64_t seed = std::stoull(argv[4]);
        const auto began = std::chrono::steady_clock::now();
        Evaluator evaluate(course);
        std::vector<Candidate> candidates{{"source", identity(), evaluate(identity())}};
        for (int arg = 5; arg < argc; ++arg) {
            const Course donor(argv[arg]);
            for (int variant = 0; variant < 2; ++variant) {
                const auto layout = donor_layout(course, donor, variant ? 1. : .2, true);
                if (std::any_of(candidates.begin(), candidates.end(), [&](const auto& old) { return old.layout == layout; })) continue;
                candidates.push_back({"equal_radius_donor_" + std::to_string(arg - 5) + "_" + std::to_string(variant), layout, evaluate(layout)});
            }
        }
        const auto seeds = candidates;
        const auto best_seed = *std::min_element(seeds.begin(), seeds.end(), [](const auto& a, const auto& b) { return a.score.cost < b.score.cost; });
        candidates.push_back(swap_search(course, evaluate, best_seed, seconds, seed, false, true));
        candidates.push_back(population_search(course, evaluate, seeds, seconds, seed + 1, false, true));
        candidates.push_back(population_search(course, evaluate, seeds, seconds, seed + 2, true, true));
        std::ofstream report(output / "candidates.csv");
        report << "method,predicted_cost,predicted_workers,rejected_days,changed_final_cells,genes\n" << std::setprecision(12);
        for (const auto& candidate : candidates) {
            Timeline timeline; timeline.fill(candidate.layout);
            save_timeline(timeline, output / (candidate.method + ".timeline.txt"));
            int changed = 0; for (int i = 0; i < 100; ++i) changed += candidate.layout[i] != i;
            report << candidate.method << ',' << candidate.score.cost << ',' << candidate.score.workers << ',' << candidate.score.rejected << ',' << changed << ",0\n";
        }
        for (int variant = 0; variant < 3; ++variant) {
            const auto candidate = dated_search(course, evaluate, seconds, seed + 3 + variant, variant == 0, variant == 2);
            save_timeline(candidate.timeline, output / (candidate.method + ".timeline.txt"));
            std::ofstream genes(output / (candidate.method + ".moves.csv")); genes << "day,first,second\n";
            for (const auto& move : candidate.moves) genes << move.day << ',' << move.first << ',' << move.second << '\n';
            int changed = 0; for (int i = 0; i < 100; ++i) changed += candidate.timeline.back()[i] != i;
            report << candidate.method << ',' << candidate.score.cost << ',' << candidate.score.workers << ',' << candidate.score.rejected << ',' << changed << ',' << candidate.moves.size() << '\n';
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"seconds\":" << elapsed << ",\"layouts\":" << evaluate.queries << ",\"cache_hits\":" << evaluate.hits
                << ",\"day_queries\":" << evaluate.day_queries << ",\"day_cache_hits\":" << evaluate.day_hits << ",\"evictions\":" << evaluate.evictions << "}\n";
        std::cout << evaluate.queries << " layouts; " << elapsed << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
