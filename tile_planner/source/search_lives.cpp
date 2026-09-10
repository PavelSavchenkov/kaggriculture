#include "life_search.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 5 || argc > 7) throw std::runtime_error("usage: search_lives case new_output seconds_per_search seed [incumbent_assignment [finance_source]]");
        const auto program = load_placement_program(argv[1]); const auto& lives = program.lives; const fs::path output(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        save_placement_program(program, output / "INPUT.plan");
        std::optional<Course> finance_source;
        if (argc == 7) finance_source.emplace(argv[6]);
        LifeEvaluator evaluate(lives, program.land, true, true, false, finance_source ? &finance_source->days : nullptr);
        std::vector<LifeCandidate> candidates;
        if (argc >= 6) {
            const auto assignment = read_life_assignment(argv[5], lives.size());
            candidates.push_back({"incumbent", assignment, evaluate(assignment)});
        }
        std::ofstream unsupported(output / "unconstructed_rules.txt");
        for (const auto& name : {"row", "nearest", "animals"}) {
            const auto assignment = try_assign_lives(lives, std::string(name) == "animals", std::string(name) != "row");
            if (!assignment) { unsupported << name << '\n'; continue; }
            candidates.push_back({name, *assignment, evaluate(*assignment)});
        }
        if (candidates.empty()) throw std::runtime_error("no initial assignment could be constructed");
        const auto initial = *std::min_element(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.score.cost < b.score.cost; });
        const auto began = std::chrono::steady_clock::now();
        candidates.push_back(search_lives(lives, evaluate, initial, std::stod(argv[3]), std::stoull(argv[4]), false));
        candidates.push_back(search_lives(lives, evaluate, initial, std::stod(argv[3]), std::stoull(argv[4]) + 1, true));
        std::ofstream table(output / "candidates.csv"); table << "method,predicted_cost,predicted_workers,rejected_days,passes,neighborhood_exhausted\n" << std::setprecision(12);
        for (const auto& candidate : candidates) {
            const auto folder = output / candidate.method; fs::create_directories(folder); save_life_assignment(lives, candidate.assignment, folder);
            table << candidate.method << ',' << candidate.score.cost << ',' << candidate.score.workers << ',' << candidate.score.rejected << ',' << candidate.passes << ',' << candidate.exhausted << '\n';
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json"); summary << "{\"seconds\":" << seconds << ",\"queries\":" << evaluate.queries << ",\"cache_hits\":" << evaluate.hits << "}\n";
        std::cout << evaluate.queries << " assignments; " << seconds << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
