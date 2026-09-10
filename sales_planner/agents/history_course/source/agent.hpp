#pragma once
#include "agents/external/atakan_demand/source/base/agent.hpp"
#include "compile_calendar.hpp"
#include "purchase_repair.hpp"
#include "rival_stock.hpp"
#include "timing.hpp"
#include "day_timing.hpp"
#include "rival_delivery.hpp"
#include "holding_exchange.hpp"

namespace kag::agents::history_course {
class Agent {
    using Course = compositions::atakan_portfolio::Agent;
    int branch_;
    bool timing_;
    bool inventory_guard_;
    bool day_timing_;
    bool delivery_timing_;
    bool multiple_sales_;
    bool holding_exchange_;
    Course course_;
    kag::agent::AgentConfig config_;
    sales_planner::MarketRules rules_;
    sales_planner::CompiledCalendar compiled_;
    std::vector<sales_planner::Orders> order_plan_;
    sales_planner::RivalStockHistory history_;
    sales_planner::TimingMemory memory_;
    sales_planner::PurchaseRepairStats repairs_;
    kag::agent::AgentObservation previous_;
    sales_planner::Items sold_{};
    bool have_previous_ = false;
    double compile_seconds_ = 0;
public:
    explicit Agent(int branch = 0, bool timing = true, bool inventory_guard = false, bool day_timing = false,
                   bool delivery_timing = false, bool multiple_sales = false, bool holding_exchange = false)
        : branch_(branch), timing_(timing), inventory_guard_(inventory_guard), day_timing_(day_timing),
          delivery_timing_(delivery_timing), multiple_sales_(multiple_sales), holding_exchange_(holding_exchange), course_(branch) {
        if (branch < 0 || branch > 2) std::abort();
        if (day_timing && !timing) std::abort();
        if (delivery_timing && !day_timing) std::abort();
        if (multiple_sales && !delivery_timing) std::abort();
        if (holding_exchange && !multiple_sales) std::abort();
    }
    static kag::agent::AgentInfo info() { return {"history_course"}; }
    void reset(const kag::agent::AgentInit& init) {
        course_.reset(init); config_ = init.config;
        const auto& c = config_;
        rules_ = {c.shed_capacity, c.max_orders, c.hire_mult, c.turns_per_day,
                  c.shop_sell_interval, c.center_sell_interval};
        compiled_ = {}; order_plan_.clear(); history_ = {}; memory_ = {}; repairs_ = {};
        sold_ = {}; have_previous_ = false; compile_seconds_ = 0;
    }
    void act(const kag::agent::AgentObservation& obs,
             const kag::agent::DecisionBudget& budget, kag::Action& action) {
        using namespace sales_planner;
        if (timing_ && have_previous_) history_.observe(previous_, obs, sold_, rules_);
        course_.act(obs, budget, action);
        if (obs.step == 226 && budget.max_expansions && !budget.soft_expired() && !budget.hard_expired()) {
            const auto started = std::chrono::steady_clock::now();
            Course fixed(branch_);
            compiled_ = compile_calendar(obs, config_, fixed, 202609091234, budget);
            if (day_timing_ && compiled_.complete) order_plan_ = compiled_.warm_orders;
            compile_seconds_ = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        }
        Orders orders;
        orders.count = action.n_orders;
        std::copy_n(action.orders, orders.count, orders.values.begin());
        if (day_timing_ && compiled_.complete) orders = order_plan_[obs.step];
        PlannerObservation current;
        if (timing_ || compiled_.complete) {
            current.turn = obs.step; current.n_shops = obs.n_shops;
            current.rival_cash = obs.opponent().money;
            std::copy_n(obs.market.inventory, kag::N_PRODUCTS, current.inventory.begin());
            std::copy_n(obs.shops, obs.n_shops, current.shops.begin());
            if (!project_resources(obs, action, current.own, current.resources,
                                   rules_.capacity, rules_.turns_per_day)) std::abort();
        }
        if (compiled_.complete) {
            std::array<int, kag::N_PRODUCTS> ready{};
            uint16_t blocked = 0;
            if (timing_) {
                for (const auto& row : obs.opponent().tiles) for (const auto& tile : row) {
                    if (tile.has_animal) ready[kag::ANIMALS[tile.what - kag::GOOSE].product] += tile.yield_units;
                    else if (tile.kind == kag::T_PLANT && obs.day - tile.planted_day >= kag::CROPS[tile.what].first_yield_day)
                        ready[tile.what] += tile.yield_units;
                }
                for (int item = 0; item < kag::N_PRODUCTS; ++item)
                    if (!output_product(item) || history_.upper[item]) blocked |= uint16_t{1} << item;
                // Fulfil the previous turn's sale promise before adding new
                // repair orders. The fixed course supplies the promised slot.
                if (!day_timing_)
                    orders = delay_for_demand(current, compiled_.plan.turns, orders, {}, ready,
                        memory_, rules_, true, 0, true, blocked, true);
            }
            if (budget.max_expansions && !budget.soft_expired() && !budget.hard_expired()) {
                orders = repair_purchases(current, compiled_.plan.turns, orders, rules_, repairs_);
                if (day_timing_) {
                    order_plan_[obs.step] = orders;
                    std::array<int, kag::N_PRODUCTS> not_before{};
                    if (delivery_timing_) not_before = rival_sale_not_before(obs, history_, rules_);
                    const auto planner = holding_exchange_ ? improve_sale_holding : multiple_sales_ ? delay_sales_within_day : delay_within_day;
                    orders = planner(current,
                        compiled_.plan.turns, order_plan_, ready, blocked,
                        memory_, rules_, std::min<uint64_t>(1000, budget.max_expansions),
                        delivery_timing_ ? &not_before : nullptr);
                } else if (timing_ && obs.step + 1 < int(compiled_.warm_orders.size()))
                    orders = delay_for_demand(current, compiled_.plan.turns, orders,
                        compiled_.warm_orders[obs.step + 1], ready, memory_, rules_, true,
                        std::min<uint64_t>(20, budget.max_expansions), true, blocked, true, inventory_guard_);
            }
        }
        action.n_orders = orders.count;
        std::copy_n(orders.values.begin(), orders.count, action.orders);
        action.finalize();
        if (timing_) {
            sold_ = requested_output_sales(current.own, orders, rules_.max_orders);
            previous_ = obs; have_previous_ = true;
        }
    }
    const auto& history() const { return history_; }
    const auto& timing() const { return memory_; }
    const auto& repairs() const { return repairs_; }
    const auto& compiled() const { return compiled_; }
    double compile_seconds() const { return compile_seconds_; }
};
}
