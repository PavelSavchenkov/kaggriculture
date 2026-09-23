#pragma once
#include "forecast.hpp"
#include "inventory.hpp"
#include "schedule.hpp"

namespace kag::day_compiler {
struct InputContinuation {
    InventoryProblem::Residual scenarios[4]{};
    int minimum_requirement=0, service_requirement=0;
};
// Two explicit service scenarios crossed with zero/repeated causal rival flow.
// This is an uncalibrated baseline, never a hard herd-count reserve.
InputContinuation wheat_continuation(const Observation& observation,const History& history,
                                    const ResourceSchedule& resources,const Configuration& config={});
struct InputMarketOptions {
    int forecast=0;
    const InputContinuation* continuation=nullptr;
    bool protect_purchase_cash=false, prefer_required_purchase=false;
    bool clip_unavoidable_returns=false, buy_after_fixed_orders=false;
};
// Conditional wheat component: the supplied complete own program fixes worker
// actions and other-product orders. It is never a rival replay or future input.
class InputMarket {
public:
    ExecutionError orders(const Observation& observation,const History& history,const ResourceSchedule& resources,
                          const Action* own_program,Action& action,const InputMarketOptions& options={},const Configuration& config={});
    InventoryChoice diagnostics{};
    InventoryProblem problem_diagnostics{};
    int failure_hour=-1, capacity_shortfall=0;
    int failure_product=-1, failure_stock=0, failure_order=0;
private:
    InventoryController controller_;
};
}
