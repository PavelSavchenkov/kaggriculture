#include "cold_cases.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3) throw std::runtime_error("usage: check_life_compile_reuse manifest new_json");
        std::ifstream manifest(argv[1]); std::ofstream output(argv[2]);
        if (!manifest || !output) throw std::runtime_error("cannot open compile-reuse files");
        output << "{\"cases\":[";
        std::string path; int cases = 0, checked_lives = 0, checked_days = 0;
        while (std::getline(manifest, path)) {
            const auto program = load_placement_program(path);
            std::unordered_map<LifeSpec, int, LifeSpecHash> unique;
            for (const auto& life : program.lives) {
                ++unique[life.spec];
                const auto direct = compile_life(life.spec);
                if (direct.spec != life.spec || direct.release_day != life.release_day) throw std::runtime_error("life identity changed");
                for (int day = 0; day < 30; ++day) {
                    const auto& a = direct.days[day]; const auto& b = life.days[day];
                    if (a.before != b.before || a.after_work != b.after_work || a.after_night != b.after_night || a.work.size() != b.work.size())
                        throw std::runtime_error("cached life state changed");
                    for (size_t i = 0; i < a.work.size(); ++i) {
                        const auto& x = a.work[i]; const auto& y = b.work[i];
                        if (x.op != y.op || x.arg != y.arg || x.quantity != y.quantity || x.output_item != y.output_item || x.output_quantity != y.output_quantity)
                            throw std::runtime_error("cached life work changed");
                    }
                    ++checked_days;
                }
                ++checked_lives;
            }
            if (cases++) output << ',';
            output << "{\"path\":" << std::quoted(path) << ",\"lives\":" << program.lives.size() << ",\"oracle_calls\":" << unique.size() << '}';
        }
        output << "],\"checked_lives\":" << checked_lives << ",\"checked_days\":" << checked_days << ",\"all_states_and_work_exact\":true}\n";
        std::cout << cases << " plans, " << checked_lives << " lifetimes, " << checked_days << " days: exact oracle agreement\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
