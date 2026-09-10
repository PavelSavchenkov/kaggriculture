#pragma once
#include "sales_planner/agents/room_keep/source/agent.hpp"
#include "sale_priority.hpp"

namespace sales_planner::agents::priority {
class Agent {
    kag::agents::sales_planner_room::Agent parent_;
    int capacity_=100;
public:
    static kag::agent::AgentInfo info() { return {"sale_priority"}; }
    void reset(const kag::agent::AgentInit& init) { parent_.reset(init); capacity_=init.config.shed_capacity; }
    void act(const kag::agent::AgentObservation& obs,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        parent_.act(obs,budget,action);
        if (budget.max_expansions==0 || budget.soft_expired() || budget.hard_expired()) return;
        Account account;
        if (!project_stock(obs,action,account,capacity_)) return;
        Orders original; original.count=action.n_orders;
        std::copy_n(action.orders,action.n_orders,original.values.begin());
        std::array<int,kag::N_PRODUCTS> inventory;
        std::copy_n(obs.market.inventory,kag::N_PRODUCTS,inventory.begin());
        const auto improved=sale_priority(account,inventory,original);
        std::copy_n(improved.values.begin(),improved.count,action.orders);
        action.finalize();
    }
};
}
