#pragma once
#include "sales_planner/agents/room_keep/source/agent.hpp"
#include "immediate_sales.hpp"

namespace sales_planner::agents {
template<bool SalesOnly>
class Immediate {
    kag::agents::sales_planner_room::Agent parent_;
    MarketRules rules_;
public:
    static kag::agent::AgentInfo info() { return {SalesOnly?"immediate_sale_turns":"immediate_all_turns"}; }
    void reset(const kag::agent::AgentInit& init) {
        parent_.reset(init);
        const auto& c=init.config;
        rules_={c.shed_capacity,c.max_orders,c.hire_mult,c.turns_per_day,c.shop_sell_interval,c.center_sell_interval};
    }
    void act(const kag::agent::AgentObservation& obs,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        parent_.act(obs,budget,action);
        if (budget.max_expansions==0 || budget.soft_expired() || budget.hard_expired()) return;
        Account account;
        if (!project_stock(obs,action,account,rules_.capacity)) return;
        Orders original; original.count=action.n_orders;
        std::copy_n(action.orders,action.n_orders,original.values.begin());
        std::array<int,kag::N_PRODUCTS> inventory;
        std::copy_n(obs.market.inventory,kag::N_PRODUCTS,inventory.begin());
        const auto improved=immediate_sales(account,inventory,original,rules_,SalesOnly);
        action.n_orders=improved.count;
        std::copy_n(improved.values.begin(),improved.count,action.orders);
        action.finalize();
    }
};
using Agent=Immediate<true>;
using AllTurns=Immediate<false>;
}
