#include "life_search.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 2 || fs::exists(argv[1])) throw std::runtime_error("usage: check_repeated_service new_output");
        const fs::path output(argv[1]); fs::create_directories(output);
        int checked = 0;
        auto spec = productive_life(kag::WHEAT, 0);
        spec.fertilize = 1u << 2; spec.service_order[2] = 0x166;
        bool rejected = false;
        try { compile_life(spec); } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("legacy spec accepted repeated service");
        ++checked;
        spec.repeated_service = true;
        auto life = compile_life(spec);
        const auto& work = life.days[2].work;
        if (std::count_if(work.begin(), work.end(), [](const auto& a) { return a.op == kag::OP_FERTILIZE; }) != 2)
            throw std::runtime_error("repeated fertilizer consumption lost");
        ++checked;
        PlacementProgram program; program.lives.push_back(life);
        save_placement_program(program, output / "valid.plan");
        const auto loaded = load_placement_program((output / "valid.plan").string());
        if (loaded.lives.size() != 1 || loaded.lives[0].spec != life.spec) throw std::runtime_error("version-four spec round trip changed");
        ++checked;
        for (int d = 0; d < 30; ++d) {
            const auto& a = life.days[d]; const auto& b = loaded.lives[0].days[d];
            if (a.before != b.before || a.after_work != b.after_work || a.after_night != b.after_night || a.work.size() != b.work.size())
                throw std::runtime_error("version-four biology round trip changed");
            for (size_t i = 0; i < a.work.size(); ++i) {
                const auto& x = a.work[i]; const auto& y = b.work[i];
                if (x.op != y.op || x.arg != y.arg || x.quantity != y.quantity || x.output_item != y.output_item || x.output_quantity != y.output_quantity)
                    throw std::runtime_error("version-four work round trip changed");
            }
            ++checked;
        }
        auto missing = spec; missing.service_order[2] = 0x66;
        rejected = false;
        try { compile_life(missing); } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("repeated service allowed a missing water obligation");
        ++checked;
        program.lives[0].spec.repeated_service = false;
        save_placement_program(program, output / "invalid_legacy.plan");
        rejected = false;
        try { load_placement_program((output / "invalid_legacy.plan").string()); } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("legacy file accepted repeated service");
        ++checked;
        LifeEvaluator plain(loaded.lives, {}, false, false, true), cached(loaded.lives);
        for (int cell : {44, 34, 23, 0}) {
            const auto a = plain({cell}), b = cached({cell});
            if (a.day_cost != b.day_cost || a.day_workers != b.day_workers || a.lower != b.lower || a.rejected != b.rejected)
                throw std::runtime_error("repeated-service caching changed a prediction");
            ++checked;
        }
        std::ofstream report(output / "SUMMARY.json"); report << "{\"checks\":" << checked << ",\"passed\":true}\n";
        std::cout << checked << " repeated-service checks passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
