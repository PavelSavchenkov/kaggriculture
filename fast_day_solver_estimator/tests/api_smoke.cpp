#include "fast_day_solver_estimator/estimator.hpp"
#include <iostream>

int main() {
    day_solver::DayProblem p;
    day_solver::ManagedTile tile; tile.x = tile.y = 4; tile.state.kind = day_solver::ManagedTileKind::EMPTY;
    p.start.managed_tiles.push_back(tile); p.start.seeds[kag::WHEAT] = 1;
    day_solver::TileWorkAction plant, water; plant.op = kag::OP_PLANT; plant.arg = kag::WHEAT; water.op = kag::OP_WATER;
    p.tile_work.push_back({0, {plant, water}});
    const auto a = fast_day_solver_estimator::estimate_day(p);
    if (a.analytically_rejected || !std::isfinite(a.cost) || a.workers < a.lower) return 1;
    p.worker_count = 40;
    const auto b = fast_day_solver_estimator::estimate_day(p);
    if (a.cost != b.cost || a.workers != b.workers || a.features != b.features) return 2;
    if (fast_day_solver_estimator::marginal_cost(a, b) != 0) return 3;
    p.start.seeds[kag::WHEAT] = 0;
    const auto rejected = fast_day_solver_estimator::estimate_day(p);
    if (!rejected.analytically_rejected || fast_day_solver_estimator::marginal_cost(a, rejected)) return 4;
    std::cout << "Pure C++ API, answer isolation and rejected-marginal controls passed.\n";
}
