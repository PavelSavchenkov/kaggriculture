#pragma once
#include "day_solver/scheduler.hpp"
#include "components/native_solver_api/internal_hint.hpp"

namespace day_scheduler {
struct EventRouteOptions {
    double seconds = 30;
    int neighbors = 16, pickup_slots = 1, deposit_slots = 2;
    bool earliest_first = false;
    bool bundle_tiles = false;
    int tuning = 0;
};
// Joint task ownership, route order and timed shed transfers. An internal
// alternative constructor; bounded visit slots/arc neighborhoods are explicit.
Result event_routes(const day_solver::DayProblem&, const EventRouteOptions& = {});
// Representation diagnostic only: fixes known task owners and hours. Never
// call this entry point when measuring cold solver coverage.
Result event_task_witness(const day_solver::DayProblem&, const day_native::InternalHint&, const EventRouteOptions& = {});
}
