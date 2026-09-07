#include <cstdlib>
#include <iostream>

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "Solution.h"
#include "search/LocalSearch.h"
#include "search/Relocate.h"
#include "search/neighbourhood.h"

int main() {
    using namespace pyvrp;
    const int x[] = {0, 1, 3, 2};
    Matrix<Distance> distances(4, 4);
    Matrix<Duration> durations(4, 4);
    for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 4; ++b) {
            distances(a, b) = std::abs(x[a] - x[b]);
            durations(a, b) = std::abs(x[a] - x[b]);
        }
    ProblemData data({Location(0, 0), Location(1, 0), Location(3, 0), Location(2, 0)},
                     {Client(1, {1}, {}, 1), Client(2, {1}, {}, 1), Client(3, {1}, {}, 1)},
                     {Depot(0)}, {VehicleType(1, {3}, 0, 0, 0, 0, 24)}, {distances}, {durations});
    Solution initial(data, std::vector<std::vector<std::size_t>>{{1, 0, 2}});
    search::PerturbationManager perturbation;
    search::LocalSearch search(data, search::computeNeighbours(data, {}), perturbation);
    search::Relocate<1> relocate(data);
    search.addOperator(relocate);
    CostEvaluator costs({100}, 100, 100);
    const auto result = search(initial, costs, true);
    if (!result.isFeasible() || initial.distance() != 8 || result.distance() != 6) {
        std::cerr << "native routing check failed\n";
        return 1;
    }
    std::cout << "Native PyVRP local search: feasible route distance 8 -> 6\n";
}
