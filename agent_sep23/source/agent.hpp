#pragma once
#include "agents/common/api/agent_api.hpp"
#include "inference.hpp"
#include "day_compiler/source/recommended.hpp"

namespace kag::agents::agent_sep23 {
namespace dc=kag::day_compiler;
struct DayReport {
    int day=-1,raw_status=-1,raw_intent_error=0,attempts=0,selected=-1,execution_errors=0;
    int requested_missing=0,executed_missing=0,emergency_hours=0,repairs=0,repair_failures=0;
    bool raw_forecast_fallback=false,forecast_fallback=false;
    uint64_t compile_worker_attempts=0,max_step_attempts=0;
    int raw_global[9]{},executed_global[9]{};
    double inference_ms=0,compile_ms=0,execute_ms=0;
};
class Agent {
public:
    Agent();
    explicit Agent(std::shared_ptr<const bc::Model> model);
    static agent::AgentInfo info();
    void reset(const agent::AgentInit& init);
    void act(const agent::AgentObservation& observation,const agent::DecisionBudget& budget,Action& action);
    void observe_past(const agent::AgentObservation& observation,const Action& action);
    void finish(const agent::AgentObservation& observation);
    void set_recovery(int mode) { recovery_=mode; }
    void set_collections(bool enabled) { collections_=enabled; }
    void set_trust_executor(bool enabled) { trust_executor_=enabled; }
    void set_late_model(std::shared_ptr<const bc::Model> model,int day) {
        if(day<0||day>30)std::abort();
        late_model_=std::move(model);late_day_=day;
    }
    const std::array<DayReport,30>& reports() const { return reports_; }
    const dc::History& history() const { return history_; }
    bc::Features features(const agent::AgentObservation& observation,const dc::IntentSchema& schema) const;
    const dc::RepairDiagnostics& repair_diagnostics() const { return executor_->diagnostics(); }
    const dc::DaySchedule& current_schedule() const { return executor_->schedule(); }
    const dc::CompileDiagnostics& compile_diagnostics() const { return schedule_->diagnostics; }
    bool compile_intent(const agent::AgentObservation& observation,const dc::DayIntent& intent,
                        const agent::DecisionBudget& budget,int candidate=0,uint64_t attempt_limit=UINT64_MAX,
                        bool require_stressed_funding=false);
    void execute(const agent::AgentObservation& observation,const agent::DecisionBudget& budget,Action& action);
private:
    void observe(const agent::AgentObservation& observation);
    void begin_day(const agent::AgentObservation& observation);
    void emergency(const agent::AgentObservation& observation,Action& action) const;
    dc::DayIntent conservative(const agent::AgentObservation& observation,const dc::IntentSchema& schema,int mode) const;
    std::shared_ptr<const bc::Model> model_;
    std::shared_ptr<const bc::Model> late_model_;
    int late_day_=30;
    std::unique_ptr<bc::Scratch> scratch_;
    std::unique_ptr<dc::DayCompiler> compiler_;
    std::unique_ptr<dc::DaySchedule> schedule_;
    std::unique_ptr<dc::ReactiveExecutor> executor_;
    dc::IntentBinder binder_;
    dc::History history_;
    bc::Trajectory trajectory_;
    dc::Configuration config_;
    dc::CommitmentAudit requested_audit_,executed_audit_;
    std::mt19937_64 random_{2301};
    std::array<DayReport,30> reports_{};
    int day_=-1,recovery_=1,observed_step_=-1;
    int budget_step_=-1;
    uint64_t attempts_used_=0;
    bool active_=false,requested_bound_=false,day_open_=false;
    bool collections_=true;
    bool trust_executor_=true;
};
static_assert(agent::LocalAgent<Agent>);
}
