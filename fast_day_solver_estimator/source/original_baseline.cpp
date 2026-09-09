#include "original_labor.hpp"
#include "estimate.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: original_baseline manifest.txt output.csv");
        std::mt19937 random(909); int checked = 0;
        constexpr int items[] = {kag::WHEAT,kag::CARROT,kag::TOMATO,kag::STRAWBERRY,kag::MELON,kag::GOOSE,kag::COW,kag::SHEEP};
        for (int trial = 0; trial < 500; ++trial) {
            std::vector<compositions::Life> lives;
            for (int i = 0, n = 1 + random() % 40; i < n; ++i)
                lives.push_back({items[random() % 8], int(random() % 25) * 24, 719, int(random() % 10), int(random() % 10)});
            compositions::EstimateOptions options;
            options.service = trial & 1 ? compositions::ServiceModel::Fertilized : compositions::ServiceModel::Productive;
            const auto original = compositions::estimate_plan(lives, {}, options);
            std::stable_sort(lives.begin(), lives.end(), [](const auto& a, const auto& b) { return a.start < b.start; });
            std::array<labor::original::Aggregate, 30> aggregate{};
            for (const auto& life : lives) {
                compositions::Cohort cohort{uint8_t(life.item),1,life.start/24,std::min(30,(life.end+23)/24)};
                const auto biological = compositions::biology(cohort, compositions::productive_service(cohort, trial & 1));
                const int distance = compositions::shed_distance(life.x, life.y);
                for (int d = 0; d < 30; ++d) {
                    const auto& b = biological.days[d]; auto& a = aggregate[d];
                    a.operations += b.operations;
                    if (b.operations) a.farthest = std::max(a.farthest, distance);
                    a.distance_work += options.travel_per_operation * b.operations;
                    const int output = std::accumulate(std::begin(b.output), std::end(b.output), 0);
                    a.distance_work += (output + b.wheat + b.fertilizer) * double(2 * distance + 1) / options.deposit_bundle;
                }
            }
            int total = 0;
            for (int day = 0; day < 30; ++day) {
                const auto result = labor::original::estimate(aggregate[day], options.max_hands);
                if (result.work_turns != original.work_turns[day] || result.workers != 1 + original.hands[day])
                    throw std::runtime_error("original formula parity failed");
                total += result.cost; ++checked;
            }
            if (total != original.hire_cost) throw std::runtime_error("original hire-cost parity failed");
        }
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("invalid manifest/output");
        output << "id,operations,work_turns,workers,hire_cost,flat_10_from_zero,flat_25_from_zero,microseconds\n" << std::setprecision(10);
        std::string id, path; int count = 0;
        while (input >> id >> path) {
            const auto p = day_solver::load_problem_json(path);
            const auto start = std::chrono::steady_clock::now();
            const auto a = labor::original::aggregate(p); const auto e = labor::original::estimate(a);
            const auto end = std::chrono::steady_clock::now();
            output << id << ',' << a.operations << ',' << e.work_turns << ',' << e.workers << ',' << e.cost << ','
                << labor::original::marginal_flat(a.operations, 0, 10) << ',' << labor::original::marginal_flat(a.operations, 0, 25)
                << ',' << std::chrono::duration<double,std::micro>(end-start).count() << '\n';
            ++count;
        }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::cout << "Exact original labor parity: " << checked << " generated composition-days; adapted physical inputs: " << count << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
