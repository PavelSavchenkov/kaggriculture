#include "timeline_search.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3) throw std::runtime_error("usage: check_timeline DeeperNet_course output_timeline");
        const Course course(argv[1]);
        const auto timeline = make_timeline(course, {{7, 25, 36}});
        auto invalid = timeline; std::swap(invalid[8][25], invalid[8][36]);
        if (legal_timeline(course, invalid)) throw std::runtime_error("accepted moving an existing animal");
        auto duplicate = timeline; duplicate[10][25] = duplicate[10][36];
        if (legal_timeline(course, duplicate)) throw std::runtime_error("accepted duplicate location");
        Evaluator cached(course), plain(course, false);
        const auto a = cached.series(timeline), b = plain.series(timeline);
        if (a.day_cost != b.day_cost || a.day_workers != b.day_workers || a.lower != b.lower) throw std::runtime_error("dated cache disagreement");
        save_timeline(timeline, argv[2]);
        if (read_timeline(argv[2]) != timeline) throw std::runtime_error("timeline serialization disagreement");
        std::cout << "dated empty-tile swap valid; existing-animal and duplicate swaps rejected; cached estimates agree\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
