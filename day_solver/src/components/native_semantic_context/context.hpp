#pragma once
#include "day_solver_api.hpp"
#include "../native_semantic_candidates/candidates.hpp"

namespace day_native::semantic {
// Owned candidate-generation data, compiled only from public v3 input.
// Delivery assignment and scarcity are search heuristics, never exact bounds.
day_semantic::Context prepare(const day_solver::DayProblem& problem);
}
