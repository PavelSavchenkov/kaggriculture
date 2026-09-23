#pragma once
#include "compiler.hpp"
#include "remaining.hpp"

namespace kag::day_compiler {
struct RepairOptions {
    int max_candidates=8;
    uint64_t max_worker_attempts=128, attempts_per_candidate=16;
    double milliseconds=50;
    bool monitor_funding=false;
    bool defer_newborn_service=true;
    // An optional proactive replacement must also fund the remaining work
    // under the existing public-supply scenario, not just the weaker model.
    bool cover_projected_supply=true;
};
struct RepairDiagnostics {
    int triggers=0, repaired=0, failed=0, candidates=0, preserved=0, worker_attempts=0, changed_fills=0, progress_failures=0;
    ExecutionError trigger=ExecutionError::None;
    double milliseconds=0;
    int funding_checks=0, projected_failures=0;
    int deferred_newborn_service=0;
    int retimed_seed_purchases=0;
    double funding_check_ms=0;
};
// Retains the dawn binding and audit while recovering from actual changed
// workers, inventory or funding. Each replacement is checked to the day's end.
class ReactiveExecutor {
public:
    bool begin(const Observation& dawn,const DaySchedule& schedule);
    ExecutionError act(const Observation& current,const History& history,Action& action,
                       const Configuration& config={},const RepairOptions& options={});
    CommitmentReport finish(const Observation& final) { return audit_.finish(final); }
    const RepairDiagnostics& diagnostics() const { return diagnostics_; }
    const DaySchedule& schedule() const { return plan_; }
    const DayExecutor::InputDiagnostics& input_diagnostics() const { return input_diagnostics_; }
private:
    bool funding_changed(const Observation& current,const History& history,const Configuration& config,
                         std::chrono::steady_clock::time_point deadline);
    bool repair(const Observation& current,const History& history,const Configuration& config,const RepairOptions& options,bool projected);
    DaySchedule plan_{};
    RemainingWork remaining_;
    CommitmentAudit audit_;
    DayExecutor executor_;
    worker::Solver workers_;
    RepairDiagnostics diagnostics_{};
    DayExecutor::InputDiagnostics input_diagnostics_{};
    struct Expected {
        int day=-1,hour=-1,workers=1,quadrants=1;
        int seeds[N_CROPS]{},shed[N_ITEMS]{};
    } expected_{};
    bool active_=false;
};
}
