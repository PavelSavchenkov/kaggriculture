#pragma once

#include "day_solver_api.hpp"

namespace day_solver {

struct TinyOracleOptions {
    // All later worker actions are PASS. Keep this small: the oracle enumerates
    // every successful raw action prefix and is intended only for tiny tests.
    int active_hours = 8;
    uint64_t node_limit = 1'000'000;
};

DayFrontier solve_tiny_exhaustive(const DayProblem& problem,
                                  TinyOracleOptions options = {});

}  // namespace day_solver
