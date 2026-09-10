#pragma once
#include "agents/external/early_structure_cow/source/agent.hpp"
#include "compact.hpp"
#include "project_stock.hpp"

namespace kag::agents::sales_planner_compact {
class Agent {
    kag::agents::early_structure_cow::Agent parent_;
    int capacity_ = 100;
public:
    static kag::agent::AgentInfo info() { return {"sales_planner_compact"}; }
    void reset(const kag::agent::AgentInit& init) {
        parent_.reset(init); capacity_ = init.config.shed_capacity;
    }
    void act(const kag::agent::AgentObservation& obs, const kag::agent::DecisionBudget& budget, kag::Action& action) {
        parent_.act(obs, budget, action);
        if (budget.max_expansions == 0 || budget.soft_expired() || budget.hard_expired()) return;
        sales_planner::Account account;
        if (!sales_planner::project_stock(obs, action, account, capacity_)) return;
        sales_planner::Orders original;
        original.count = action.n_orders;
        std::copy_n(action.orders, action.n_orders, original.values.begin());
        const auto improved = sales_planner::compact_orders(account, original, 1);
        action.n_orders = improved.count;
        std::copy_n(improved.values.begin(), improved.count, action.orders);
        action.finalize();
    }
};
}
