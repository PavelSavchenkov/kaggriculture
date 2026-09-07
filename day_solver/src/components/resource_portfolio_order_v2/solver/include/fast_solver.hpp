#pragma once

#include "day_solver_api.hpp"

namespace day_solver {

struct FastSolverOptions {
    uint32_t route_order_begin = 0;
    uint32_t route_orders = 512;
};

DayFrontier solve_fast(const DayProblem& problem,
                       FastSolverOptions options = {});

}  // namespace day_solver
