#pragma once

#include <string>
#include <vector>

#include "day_solver_api.hpp"

namespace day_solver {

struct ValidationIssue {
    std::string path;
    std::string message;
};

std::vector<ValidationIssue> validate_problem(const DayProblem& problem);

}  // namespace day_solver
