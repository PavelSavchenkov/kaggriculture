#pragma once
#include "day_solver/scheduler.hpp"
#include "components/native_solver_api/internal_hint.hpp"

namespace day_scheduler {
struct JobBeamStats {
    // Estimates used to choose the next search method, never to reject a day.
    int completed_partitions = 0;
    bool estimated_timing_feasible = false;
};
struct JobBeamOptions {
    double seconds = 12, completion_seconds = 0.4;
    int width = 32, variants = 4, completions = 8;
    int first_variant = 0;
    bool diagnose = false;
    double polish_seconds = 0;
    int max_job_tasks = 0;
    double capacity_seconds = 0;
    bool terminal_capacity = false;
    bool timed_hint = false;
    // Split a delivery job when at most one worker can finish it alone in time.
    bool urgent_fragments = false;
    bool extra_pickups = false;
    bool inventory_pickups = false;
    bool prefer_unpolished = false;
    // Try a promising raw route before spending time on its local improvement.
    bool defer_polish = false;
    double raw_completion_share = 0.75;
    bool raw_timed_hint = false;
    bool seed_deadlines = false;
    bool pickup_deadlines = false;
    // Keep original scoring on variants 0/2/3; insert a purchase-aware variant
    // after the first two different job orders.
    bool alternate_purchase_scores = false;
    bool polish_history = false;
    bool polish_chains = false;
    bool return_dp = false;
    bool regret_order = false;
    bool savings_order = false;
    int savings_rollouts = 1;
    bool cache_routes = false;
    double rebuild_seconds = 0;
    double recombine_seconds = 0;
    bool recombine_reassign = false;
    bool route_first = false;
    bool lazy_rebuild = false;
    bool adaptive_rebuild = false;
    // Optional dispatch: 1/2 serial hand/fitted; 3/4 event beam hand/fitted.
    int dispatch_order = 0;
    bool dispatch_columns = false;
    bool dispatch_lookahead = false;
    // Ranking: 0 selected early trips, 1 omit them, 2 match prefix returns.
    // Mode 3 permits repeated prefix returns on each route.
    // Timed completion always enforces every delivery.
    int delivery_score = 0;
};
// Cold insertion beam over useful-action jobs. Travel is Manhattan distance;
// the timed stock model and strict replay decide whether a route set succeeds.
Result job_beam(const day_solver::DayProblem&, const JobBeamOptions& = {}, JobBeamStats* = nullptr);
// Diagnostic only: inspect representation and score of a known task order.
// Never use this entry point in cold solver coverage or frontier measurements.
Result job_route_witness(const day_solver::DayProblem&, const day_native::InternalHint&, int max_job_tasks = 0, bool return_dp = false);
// Offline imitation examples. This API is never called by cold construction.
std::string job_dispatch_examples(const day_solver::DayProblem&, const day_native::InternalHint&);
}
