#pragma once
#include "day_solver_api.hpp"
#include <chrono>

namespace day_scheduler::storage {
constexpr int unlimited = std::numeric_limits<int16_t>::max();
struct Frame {
    std::array<day_solver::InventoryCount, kag::N_ITEMS> shed;
    std::vector<day_solver::WorkerState> workers;
};
using Frames = std::array<Frame, day_solver::HOURS>;
Frames trace(const day_solver::DayProblem&, const std::array<kag::Action, day_solver::HOURS>&);
std::optional<std::array<kag::Action, day_solver::HOURS>> repair(
    const day_solver::DayProblem&, const std::array<kag::Action, day_solver::HOURS>&,
    std::chrono::steady_clock::time_point deadline);
}
