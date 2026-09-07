#pragma once
#include "../native_solver_api_coarse/screen.hpp"
namespace day_native::geometry_screen {
using screen::ScreenOptions;
using screen::Task;
using screen::Profile;
using screen::Delivery;
using screen::DataSnapshot;
using screen::SolveOptions;
using screen::Result;
Result solve(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
}
