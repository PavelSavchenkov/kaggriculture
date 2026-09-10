#include "life_search.hpp"
#include <unordered_set>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if ((argc != 3 && argc != 4) || fs::exists(argv[2])) throw std::runtime_error("usage: check_life_day_cache input_list new_report [finance_list]");
        std::ifstream input(argv[1]); if (!input) throw std::runtime_error("cannot read input list");
        std::ifstream finance_input; if (argc == 4) { finance_input.open(argv[3]); if (!finance_input) throw std::runtime_error("cannot read finance list"); }
        std::ofstream output(argv[2]); output << "{\"cases\":[";
        std::string name; int cases = 0, checked = 0; double slow_total = 0, fast_total = 0;
        std::mt19937_64 rng(419003);
        while (std::getline(input, name)) if (!name.empty()) {
            const auto program = load_placement_program(name); const auto& lives = program.lives;
            std::optional<Course> finance;
            if (argc == 4) {
                std::string path;
                if (!std::getline(finance_input, path) || path.empty()) throw std::runtime_error("short finance list");
                finance.emplace(path);
            }
            const LifeEquivalence equivalence(lives);
            const auto supplied = fs::path(name).parent_path() / "assignment.txt";
            const auto initial = fs::is_regular_file(name) && fs::exists(supplied) ? read_life_assignment(supplied, lives.size()) : assign_lives(lives, true, true);
            auto current = initial;
            std::vector<LifeAssignment> assignments; std::unordered_set<LifeAssignment, AssignmentHash> seen;
            for (int attempt = 0; attempt < 30000 && assignments.size() < 256; ++attempt) {
                auto trial = attempt % 4 ? current : initial;
                const int first = rng() % lives.size(), second = rng() % lives.size(), cell = rng() % 100, origin = trial[first];
                switch (rng() % 4) {
                    case 0: trial[first] = cell; break;
                    case 1: std::swap(trial[first], trial[second]); break;
                    case 2:
                        for (size_t i = 0; i < lives.size(); ++i) if (trial[i] == origin && kag::is_crop(lives[i].spec.item) && lives[i].spec.begin_day >= lives[first].spec.begin_day) trial[i] = cell;
                        break;
                    case 3:
                        if (quadrant(origin) != quadrant(cell)) continue;
                        for (int& position : trial) { if (position == origin) position = cell; else if (position == cell) position = origin; }
                        break;
                }
                equivalence.normalize(trial);
                if (!legal_lives(lives, trial) || !seen.insert(trial).second) continue;
                current = trial; assignments.push_back(std::move(trial));
            }
            if (assignments.empty()) throw std::runtime_error("no legal validation assignments");
            std::vector<Score> expected(assignments.size()), actual(assignments.size());
            LifeEvaluator slow(lives, program.land, true, false, true, finance ? &finance->days : nullptr), fast(lives, program.land, true, true, false, finance ? &finance->days : nullptr);
            auto measure = [&](LifeEvaluator& evaluator, std::vector<Score>& scores) {
                const auto began = std::chrono::steady_clock::now();
                for (size_t i = 0; i < assignments.size(); ++i) scores[i] = evaluator(assignments[i]);
                return std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
            };
            double slow_seconds, fast_seconds;
            if (cases % 2) { fast_seconds = measure(fast, actual); slow_seconds = measure(slow, expected); }
            else { slow_seconds = measure(slow, expected); fast_seconds = measure(fast, actual); }
            for (size_t i = 0; i < assignments.size(); ++i) {
                const auto& a = expected[i]; const auto& b = actual[i];
                if (a.cost != b.cost || a.workers != b.workers || a.day_cost != b.day_cost || a.day_workers != b.day_workers || a.lower != b.lower || a.rejected != b.rejected)
                    throw std::runtime_error("lifetime day cache changed an exact prediction");
            }
            if (cases++) output << ',';
            output << "{\"name\":\"" << name << "\",\"assignments\":" << assignments.size() << ",\"slow_seconds\":" << slow_seconds << ",\"fast_seconds\":" << fast_seconds
                   << ",\"day_queries\":" << fast.day_queries << ",\"day_hits\":" << fast.day_hits << ",\"evictions\":" << fast.day_evictions << '}'; output.flush();
            checked += assignments.size(); slow_total += slow_seconds; fast_total += fast_seconds;
        }
        if (argc == 4 && std::getline(finance_input, name)) throw std::runtime_error("extra finance list content");
        output << "],\"checked\":" << checked << ",\"bit_exact\":true,\"fixed_finance\":" << (argc == 4 ? "true" : "false") << ",\"slow_seconds\":" << slow_total << ",\"fast_seconds\":" << fast_total << "}\n";
        std::cout << checked << " distinct assignments: exact agreement; " << slow_total << " to " << fast_total << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
