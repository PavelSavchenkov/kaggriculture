#pragma once
#include "day_solver/scheduler.hpp"

namespace day_scheduler {
// Experimental cold constructor. Proposals may violate routing approximations;
// only complete schedules accepted by strict replay are returned.
Result fast_routes(const day_solver::DayProblem&, double seconds, int profile = 0, int iterations = 64);
}
