#pragma once
#include "commitments.hpp"
#include "funding.hpp"
#include "history.hpp"
#include "input_market.hpp"
#include "market.hpp"
#include "schedule.hpp"
#include <chrono>

namespace kag::day_compiler {
enum class CompileStatus { Success, InvalidIntent, NoFundedSchedule, Budget };
struct CompileOptions {
    int initial_hires=-1; // Automatic: inherited workload estimate; terminal starts at 13.
    int max_candidates=12;
    uint64_t max_worker_attempts=256;
    uint64_t attempts_per_candidate=32;
    double milliseconds=150;
    MarketMode market=MarketMode::SellerTerminalInventoryFlow;
    bool cover_visible_funding=true;
    bool cover_visible_wheat=true;
    bool cover_visible_fertilizer=true;
    bool fallback_to_model=true;
    bool today_first_binding=true;
    bool refine_terminal_wheat=true;
    bool trace_candidates=false;
    bool adapt_terminal_returns=true;
    bool sales_first=true;
    bool refine_ordinary_wheat=false;
    bool refine_ordinary_sales=false;
    bool refine_ordinary_collections=false;
};
struct CompileDiagnostics {
    IntentError intent_error=IntentError::None;
    ExecutionError execution_error=ExecutionError::None;
    int candidates=0, worker_attempts=0, funding_rejections=0, worker_failures=0, semantic_failures=0;
    int duplicate_proposals=0;
    int newborn_first_yield_shortfall=0;
    bool covers_visible_funding=false, forecast_fallback=false, fertilizer_fallback=false;
    bool covers_visible_fertilizer=false, fertilizer_refinement_tried=false, fertilizer_refinement_kept=false;
    bool wheat_refinement_tried=false, wheat_refinement_kept=false;
    double wheat_refinement_ms=0;
    bool ordinary_trading_tried=false, ordinary_trading_kept=false;
    int ordinary_wheat_applied=0, ordinary_wheat_fallbacks=0, ordinary_wheat_incomplete=0;
    double ordinary_trading_ms=0, ordinary_trading_settled_margin=0;
    bool collection_tried=false, collection_kept=false;
    int collection_added=0, collection_extra_units=0;
    double collection_ms=0, collection_score=0;
    int failure_hour=-1, hires=0;
    double milliseconds=0, projected_cash=0, projected_margin=-1e100;
};
struct DaySchedule {
    detail::BoundIntent intent{};
    worker::DayInput worker_input{};
    worker::SolveOptions worker_constraints{};
    ResourceSchedule resources{};
    Action actions[24]{};
    Action input_program[24]{}; // Frozen causal baseline for conditional other-product orders.
    CompileDiagnostics diagnostics{};
    MarketMode market=MarketMode::Liquidate;
    bool verified=false, sales_first=false, ordinary_wheat=false, ordinary_sales=false;
    DaySchedule();
};
// Rebuild market decisions hourly while retaining the verified worker schedule.
// Errors are explicit; unsupported suffixes are never silently accepted.
class DayExecutor {
public:
    struct InputDiagnostics {
        bool attempted=false, applied=false, incomplete=false, funding_deferred=false;
        bool sales_applied=false, sales_fallback=false;
        ExecutionError error=ExecutionError::None;
        uint64_t transitions=0;
    };
    ExecutionError act(const Observation& observation,const History& history,const DaySchedule& schedule,
                       Action& action,const Configuration& config={});
    const MarketPlanner::Failure& market_failure() const { return market_.failure; }
    const InventoryCurve& inventory_diagnostics() const { return market_.inventory_diagnostics; }
    const InputDiagnostics& input_diagnostics() const { return input_diagnostics_; }
private:
    MarketPlanner market_;
    InputMarket input_market_;
    InputDiagnostics input_diagnostics_{};
};
class DayCompiler {
public:
    CompileStatus compile(const Observation& dawn,const History& history,const DayIntent& intent,DaySchedule& out,
                          const Configuration& config={},const CompileOptions& options={});
private:
    CompileStatus compile_base(const Observation& dawn,const History& history,const DayIntent& intent,DaySchedule& out,
                              const Configuration& config,const CompileOptions& options);
    CompileStatus compile_once(const Observation& dawn,const History& history,const DayIntent& intent,DaySchedule& out,
                               const Configuration& config,const CompileOptions& options);
    void refine_wheat(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                      const CompileOptions& options,std::chrono::steady_clock::time_point deadline);
    void refine_funding(const Observation& dawn,const History& history,const DayIntent& intent,DaySchedule& out,
                        const Configuration& config,const CompileOptions& options,std::chrono::steady_clock::time_point deadline);
    void refine_inputs(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                       const CompileOptions& options,std::chrono::steady_clock::time_point deadline);
    void refine_collections(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                            const CompileOptions& options,std::chrono::steady_clock::time_point deadline);
    IntentBinder binder_;
    WorkloadBuilder workloads_;
    worker::Solver workers_;
    DayExecutor executor_;
    std::array<FundingProposal,12> tried_{};
};
}
