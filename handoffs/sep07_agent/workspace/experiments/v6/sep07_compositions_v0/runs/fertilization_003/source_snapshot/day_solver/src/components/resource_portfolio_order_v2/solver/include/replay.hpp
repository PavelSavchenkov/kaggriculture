#pragma once

#include <array>
#include <string>
#include <vector>

#include "day_solver_api.hpp"

namespace day_solver {

struct ReplayResult {
    DayCandidate candidate;
    bool requirements_satisfied = false;
    bool invariants_satisfied = false;
    std::vector<std::string> errors;
};

ReplayResult replay_schedule(
    const DayProblem& problem,
    const std::array<kag::Action, HOURS>& actions);

}  // namespace day_solver
