#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "day_solver_api.hpp"

namespace day_solver {

DayProblem parse_problem_json(std::string_view text);
std::string serialize_problem_json(const DayProblem& problem);
DayProblem load_problem_json(const std::filesystem::path& path);
void save_problem_json(const DayProblem& problem,
                       const std::filesystem::path& path);

}  // namespace day_solver
