#pragma once

#include <array>
#include <filesystem>

#include "day_solver_api.hpp"

namespace day_solver {

std::array<kag::Action, HOURS> load_baseline_json(
    const std::filesystem::path& path);

}  // namespace day_solver
