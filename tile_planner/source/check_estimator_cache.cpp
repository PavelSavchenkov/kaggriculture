#include "search.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 4) throw std::runtime_error("usage: check_estimator_cache output_json layouts_per_course course...");
        const int count = std::stoi(argv[2]);
        double plain_seconds = 0, cached_seconds = 0;
        int checked = 0, day_queries = 0, day_hits = 0;
        for (int arg = 3; arg < argc; ++arg) {
            const Course course(argv[arg]);
            std::mt19937_64 rng(104729 + arg);
            std::vector<Layout> layouts{identity()};
            for (int i = 1; i < count; ++i) {
                auto layout = layouts.back();
                int a, b;
                do { a = rng() % 100; b = rng() % 100; } while (course.fixed[a] || course.fixed[b] || quadrant(a) != quadrant(b));
                std::swap(layout[a], layout[b]); layouts.push_back(layout);
            }
            std::vector<Score> expected;
            const auto begin_plain = std::chrono::steady_clock::now();
            Evaluator plain(course, false);
            for (const auto& layout : layouts) expected.push_back(plain(layout));
            plain_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - begin_plain).count();
            const auto begin_cached = std::chrono::steady_clock::now();
            Evaluator cached(course);
            for (int i = 0; i < count; ++i) {
                const auto value = cached(layouts[i]); const auto& reference = expected[i];
                if (value.cost != reference.cost || value.workers != reference.workers || value.rejected != reference.rejected || value.weak != reference.weak ||
                    value.day_cost != reference.day_cost || value.day_workers != reference.day_workers || value.lower != reference.lower)
                    throw std::runtime_error("cache changed prediction");
                ++checked;
            }
            cached_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - begin_cached).count();
            day_queries += cached.day_queries; day_hits += cached.day_hits;
        }
        std::ofstream output(argv[1]);
        output << "{\"courses\":" << argc - 3 << ",\"layouts\":" << checked << ",\"bit_exact\":true,\"plain_seconds\":" << plain_seconds
               << ",\"cached_seconds\":" << cached_seconds << ",\"day_queries\":" << day_queries << ",\"day_hits\":" << day_hits << "}\n";
        std::cout << checked << " exact matches; uncached " << plain_seconds << "s, cached " << cached_seconds << "s\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
