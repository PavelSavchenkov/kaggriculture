#include "cold_cases.hpp"
#include "schedule_bank.hpp"
#include <iomanip>
#include <sstream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3) throw std::runtime_error("usage: profile_input_loading manifest new_json");
        std::ifstream manifest(argv[1]); std::ofstream output(argv[2]);
        if (!manifest || !output) throw std::runtime_error("cannot open profile files");
        output << "{\"repeats\":5,\"cases\":[";
        std::string line; bool comma = false;
        while (std::getline(manifest, line)) {
            std::istringstream fields(line); std::string name, plan, bank_path, finance_path, extra;
            if (!(fields >> name >> plan >> bank_path >> finance_path) || fields >> extra) throw std::runtime_error("invalid profile manifest row");
            double biology = 0, bank_seconds = 0, finance_seconds = 0;
            size_t lives = 0, unique = 0;
            for (int repeat = 0; repeat < 5; ++repeat) {
                auto began = std::chrono::steady_clock::now();
                const auto program = load_placement_program(plan);
                biology += std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
                lives = program.lives.size();
                if (!repeat) {
                    std::vector<LifeSpec> specs;
                    for (const auto& life : program.lives) if (std::find(specs.begin(), specs.end(), life.spec) == specs.end()) specs.push_back(life.spec);
                    unique = specs.size();
                }
                began = std::chrono::steady_clock::now();
                ScheduleBank bank; bank.load(bank_path);
                bank_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
                began = std::chrono::steady_clock::now();
                const Course finance(finance_path);
                finance_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
            }
            if (comma) output << ',';
            comma = true;
            output << "{\"name\":" << std::quoted(name) << ",\"lives\":" << lives << ",\"unique_specs\":" << unique
                   << ",\"biology_seconds\":" << biology / 5 << ",\"packed_bank_seconds\":" << bank_seconds / 5 << ",\"finance_source_seconds\":" << finance_seconds / 5 << '}';
        }
        output << "]}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
