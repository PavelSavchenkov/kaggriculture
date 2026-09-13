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
    // Diagnostic only, populated with include_data after a strict rejection.
    std::optional<std::array<kag::Action, 24>> rejected_schedule;
};
Result solve(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
Result solve_stock(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
Result solve_capacity(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
// Preserve the proposal allowance; limit total time if refinement is needed.
Result solve_capacity_budgeted(const day_solver::DayProblem&, const InternalHint&, const SolveOptions&, int tuning, double refinement_seconds);
// Fixed routes with cargo insertion order and no daytime overflow; may discard
// at night. Exposed separately for focused model regressions and profiling.
Result solve_capacity_ordered(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
// Permit extra stock on existing pickup visits to make room for purchases.
Result solve_capacity_buffered(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
// Extra carried inputs can also change the retained mix during night overflow.
Result solve_capacity_inventory(const day_solver::DayProblem&, const InternalHint&, const SolveOptions& = {}, int tuning = 1);
}
