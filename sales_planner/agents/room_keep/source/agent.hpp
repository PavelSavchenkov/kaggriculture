#pragma once
#include "agents/external/early_structure_cow/source/agent.hpp"
#include "compact.hpp"
#include "project_stock.hpp"
#include "project_resources.hpp"
#include "storage.hpp"

namespace kag::agents::sales_planner_room {
class Agent {
    kag::agents::early_structure_cow::Agent parent_;
    sales_planner::MarketRules rules_;
public:
    static kag::agent::AgentInfo info() { return {"sales_planner_room"}; }
    void reset(const kag::agent::AgentInit& init) {
        parent_.reset(init);
        const auto& c = init.config;
        rules_ = {c.shed_capacity, c.max_orders, c.hire_mult, c.turns_per_day,
                  c.shop_sell_interval, c.center_sell_interval};
    }
    void act(const kag::agent::AgentObservation& obs, const kag::agent::DecisionBudget& budget, kag::Action& action) {
        parent_.act(obs, budget, action);
        if (budget.max_expansions == 0 || budget.soft_expired() || budget.hard_expired()) return;
        sales_planner::Account account;
        if (!sales_planner::project_stock(obs, action, account, rules_.capacity)) return;
        sales_planner::Orders original;
        original.count = action.n_orders;
        std::copy_n(action.orders, action.n_orders, original.values.begin());
        auto improved = sales_planner::compact_orders(account, original, 1);
        if (obs.hour + 1 == rules_.turns_per_day) {
            sales_planner::PlannerObservation forecast;
            forecast.turn = obs.step; forecast.n_shops = obs.n_shops;
            std::copy_n(obs.shops, obs.n_shops, forecast.shops.begin());
            std::copy_n(obs.market.inventory, kag::N_PRODUCTS, forecast.inventory.begin());
            forecast.rival_cash = obs.opponent().money;
            if (sales_planner::project_resources(obs, action, forecast.own, forecast.resources,
                                                rules_.capacity, rules_.turns_per_day)) {
                std::array<sales_planner::ResourceEvent, kag::MAX_UNITS> deposits;
                for (int u = 0; u < obs.self().n_units; ++u)
                    deposits[u] = {sales_planner::Flow::drop_all, 0, uint8_t(u), 0};
                sales_planner::StorageStats stats;
                improved = sales_planner::storage_orders(forecast,
                    std::span<const sales_planner::ResourceEvent>(deposits.data(), obs.self().n_units),
                    improved, rules_, stats);
            }
        }
        action.n_orders = improved.count;
        std::copy_n(improved.values.begin(), improved.count, action.orders);
        action.finalize();
    }
};
}
