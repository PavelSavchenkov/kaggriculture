#pragma once
#include "../native_solver_api/exact.hpp"
namespace day_native::light {
using exact::HintOptions;
using exact::SolveOptions;
using exact::Result;
using exact::ReplayCheck;
Result solve(const day_solver::DayProblem&, const HintOptions&, const SolveOptions& = {}, int tuning = 1);
}
