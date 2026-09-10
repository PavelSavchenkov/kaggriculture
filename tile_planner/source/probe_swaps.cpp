#include "search.hpp"
#include "route_repair.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3) throw std::runtime_error("usage: probe_swaps course new_output");
        const Course course(argv[1]);
        const fs::path output(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        Evaluator evaluate(course); const auto baseline = evaluate(identity());
        std::array<int, 100> role{}; role.fill(-1);
        for (int cell = 0; cell < 100; ++cell) {
            std::array<int, kag::N_ITEMS> days{};
            for (const auto& day : course.days) {
                const int animal = day.problem.start.managed_tiles[cell].state.animal;
                if (animal >= 0) ++days[animal];
            }
            const auto best = std::max_element(days.begin(), days.end());
            if (*best) role[cell] = best - days.begin();
        }
        std::ofstream report(output / "swaps.csv");
        report << "name,cow_cell,sheep_cell,cow_radius,sheep_radius,predicted_cost,delta,retained_days,rerouted_days,retained_bill,weak_days\n";
        int count = 0;
        for (int cow = 0; cow < 100; ++cow) if (role[cow] == kag::COW && !course.fixed[cow])
            for (int sheep = 0; sheep < 100; ++sheep) if (role[sheep] == kag::SHEEP && !course.fixed[sheep] && quadrant(cow) == quadrant(sheep)) {
                auto layout = identity(); std::swap(layout[cow], layout[sheep]);
                const auto score = evaluate(layout);
                int retained = 0, rerouted = 0; int64_t bill = 0;
                for (int d = 0; d < 30; ++d) {
                    const auto replay = day_solver::replay_schedule(course.problem(d, layout), course.days[d].physical);
                    const bool ok = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
                    retained += ok; if (ok) bill += labor::hire_cost(course.days[d].problem.worker_count);
                    if (!ok) for (int variant = 0; variant < 4; ++variant)
                        if (route_repair(course.days[d], course.problem(d, layout), layout, variant)) { ++rerouted; break; }
                }
                const auto name = "cow_" + std::to_string(cow) + "_sheep_" + std::to_string(sheep);
                std::ofstream file(output / (name + ".layout.txt"));
                for (auto cell : layout) file << int(cell) << ' ';
                file << '\n';
                report << name << ',' << cow << ',' << sheep << ',' << labor::shed_distance(cow) << ',' << labor::shed_distance(sheep) << ','
                       << score.cost << ',' << score.cost - baseline.cost << ',' << retained << ',' << rerouted << ',' << bill << ',' << score.weak << '\n';
                ++count;
            }
        std::ofstream source(output / "source.layout.txt");
        for (auto cell : identity()) source << int(cell) << ' ';
        source << '\n';
        std::cout << count << " controlled cow/sheep swaps; baseline prediction " << baseline.cost << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
