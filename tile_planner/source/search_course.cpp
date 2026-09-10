#include "search.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 5) throw std::runtime_error("usage: search_course course new_output seconds seed");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]);
        const fs::path output(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        Evaluator evaluate(course);
        auto candidates = rules(course, evaluate);
        const double seconds = std::stod(argv[3]);
        const uint64_t seed = std::stoull(argv[4]);
        const auto best_rule = *std::min_element(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.score.cost < b.score.cost;
        });
        candidates.push_back(swap_search(course, evaluate, candidates.front(), seconds / 3, seed, false));
        candidates.push_back(swap_search(course, evaluate, best_rule, seconds / 3, seed + 1, false));
        candidates.push_back(swap_search(course, evaluate, best_rule, seconds / 3, seed + 2, true));
        std::ofstream report(output / "candidates.csv");
        report << "method,predicted_cost,predicted_workers,rejected_days,weak_days\n" << std::setprecision(12);
        for (const auto& candidate : candidates) {
            std::ofstream layout(output / (candidate.method + ".layout.txt"));
            for (auto cell : candidate.layout) layout << int(cell) << ' ';
            layout << '\n';
            report << candidate.method << ',' << candidate.score.cost << ',' << candidate.score.workers << ','
                   << candidate.score.rejected << ',' << candidate.score.weak << '\n';
            std::ofstream days(output / (candidate.method + ".days.csv"));
            days << "day,predicted_cost,predicted_workers,lower\n";
            for (int d = 0; d < 30; ++d) days << d << ',' << candidate.score.day_cost[d] << ',' << candidate.score.day_workers[d] << ',' << candidate.score.lower[d] << '\n';
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream stats(output / "SUMMARY.json");
        stats << "{\"seconds\":" << elapsed << ",\"evaluated_layouts\":" << evaluate.queries << ",\"cache_hits\":" << evaluate.hits
              << ",\"source_predicted_cost\":" << candidates.front().score.cost << ",\"seed\":" << seed << "}\n";
        std::cout << "layouts " << evaluate.queries << ", seconds " << elapsed << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
