#pragma once
#include "../native_solver_api_coarse/screen.hpp"
namespace day_native::materialized_screen {
using screen::ScreenOptions;
using screen::Task;
using screen::Profile;
using screen::Delivery;
using screen::DataSnapshot;
using screen::SolveOptions;
struct Result : screen::Result {
    std::optional<std::array<kag::Action, 24>> schedule;
    double materialize_seconds = 0;
    std::string materialize_rejection;
};
Result solve(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
}
