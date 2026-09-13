#pragma once
#include "day_solver/scheduler.hpp"
#include "components/native_solver_api/internal_hint.hpp"

namespace day_scheduler {
// Internal cold-search proposal only; never an original replay route.
Result repair_partition(const day_solver::DayProblem&, const day_native::InternalHint&,
                        double seconds, double attempt_seconds = 2, int candidates = 8);
}
