#pragma once
#include "own_world.hpp"
#include "project_resources.hpp"

namespace sales_planner {
struct CompiledCalendar {
    Calendar plan;
    std::vector<Orders> warm_orders;
    Items produced{}, discarded{};
    int projections = 0, simulated_turns = 0, failed_work = 0;
    bool complete = false;
};

inline bool same_resources(const Account& a, const Resources& r, const Account& b, const Resources& s) {
    if (a.stock != b.stock || a.total != b.total || a.seeds != b.seeds ||
        r.buffers != s.buffers || r.key_count != s.key_count) return false;
    for (int u = 0; u < kag::MAX_UNITS; ++u)
        for (int k = 0; k < r.key_count[u]; ++k)
            if (r.keys[u][k] != s.keys[u][k]) return false;
    return true;
}

// Extract ordered financial effects from a supplied current worker schedule.
// Actual accepted withdrawals are recorded; later changed inventory can alter
// a literal PICKUP, so callers still verify selected plans in the full engine.
inline bool worker_calendar(const kag::agent::AgentObservation& obs, const kag::Action& action,
                            CalendarTurn& output, int capacity, int turns_per_day, int& projections) {
    auto previous = own_account(obs); auto carried = own_resources(obs);
    auto check_account = previous; auto check_resources = carried;
    auto prefix = action;
    std::fill_n(prefix.units, prefix.n_units, kag::UnitAction{});
    auto supplied = action;
    supplied.finalize();
    for (int u = 0; u < supplied.n_units; ++u)
        if (supplied.units[u].op == kag::OP_PLANT && supplied.units[u].arg < kag::N_CROPS &&
            supplied.plant_demand[supplied.units[u].arg] > obs.own.seeds[supplied.units[u].arg]) supplied.units[u] = {};
    for (int u = 0; u < action.n_units; ++u) {
        const auto work = prefix.units[u] = supplied.units[u];
        const int op = work.op;
        if (op != kag::OP_PICKUP && op != kag::OP_DROP && op != kag::OP_PLACE &&
            op != kag::OP_PLANT && op != kag::OP_HARVEST && op != kag::OP_FERTILIZE &&
            op != kag::OP_FEED && op != kag::OP_COLLECT_FERTILIZER) continue;
        Account after; Resources next;
        prefix.finalize(); ++projections;
        if (!project_resources(obs, prefix, after, next, capacity, turns_per_day)) return false;
        auto emit = [&](Flow flow, int item, int quantity) {
            output.before_market.push_back({flow, uint8_t(item), uint8_t(u), quantity});
        };
        const bool beside = kag::is_shed_adjacent(obs.self().pos_x[u], obs.self().pos_y[u], kag::BOARD);
        if (op == kag::OP_DROP && beside) emit(Flow::drop_all, 0, 0);
        else if (op == kag::OP_PLACE && work.arg < kag::N_ITEMS && beside &&
                 (!kag::is_animal(work.arg) || after.stock[work.arg] > previous.stock[work.arg])) {
            if (work.n > 0) emit(Flow::deposit, work.arg, work.n);
        } else {
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                const int delta = next.buffers[u][item] - carried.buffers[u][item];
                if (delta > 0) emit(op == kag::OP_PICKUP ? Flow::withdraw : Flow::produce, item, delta);
                if (delta < 0) emit(Flow::use, item, -delta);
            }
        }
        for (int item = 0; item < kag::N_CROPS; ++item)
            if (after.seeds[item] < previous.seeds[item]) emit(Flow::use_seed, item, previous.seeds[item] - after.seeds[item]);
        previous = after; carried = next;
    }
    apply(check_account, check_resources, output.before_market, capacity);
    if (!same_resources(check_account, check_resources, previous, carried)) return false;
    if (obs.hour + 1 == turns_per_day)
        for (int u = 0; u < obs.self().n_units; ++u)
            output.after_market.push_back({Flow::drop_all, 0, uint8_t(u), 0});
    return true;
}

// The schedule provider must be a fixed course, not a policy that reacts to
// the temporary funding balance. Unlimited nominal cash separates production
// compilation from financial feasibility. Only the later financial evaluator
// uses the real starting cash and can certify that commitments are affordable.
template<class FixedCourse>
CompiledCalendar compile_calendar(const kag::agent::AgentObservation& start,
                                   const kag::agent::AgentConfig& config, FixedCourse& course,
                                   uint64_t forecast_seed, const kag::agent::DecisionBudget& budget) {
    CompiledCalendar result;
    result.plan.start = start.step; result.plan.end = config.episode_steps - 1;
    result.plan.starting_account = own_account(start); result.plan.starting_resources = own_resources(start);
    result.plan.turns.resize(result.plan.end); result.warm_orders.resize(result.plan.end);
    auto world = own_world(start, config, forecast_seed);
    const MarketRules rules{config.shed_capacity, config.max_orders, config.hire_mult,
                            config.turns_per_day, config.shop_sell_interval, config.center_sell_interval};
    course.reset(kag::agent::runtime::make_agent_init(world, 0));
    kag::Action pass; pass.clear(); pass.n_units = 1; pass.finalize();
    while (!world.st.done) {
        if (uint64_t(result.projections + result.simulated_turns) >= budget.max_expansions ||
            ((world.st.step & 15) == 0 && (budget.soft_expired() || budget.hard_expired()))) return result;
        world.st.farms[0].money = 1e12;
        const auto obs = kag::agent::runtime::make_observation(world, 0);
        kag::Action action; course.act(obs, budget, action);
        auto& calendar = result.plan.turns[obs.step];
        if (!worker_calendar(obs, action, calendar, config.shed_capacity, config.turns_per_day, result.projections)) return result;
        auto& orders = result.warm_orders[obs.step]; orders.count = action.n_orders;
        std::copy_n(action.orders, action.n_orders, orders.values.begin());
        auto state = financial_state(world.st); auto resources = own_resources(obs);
        apply(state.accounts[0], resources, calendar.before_market, rules.capacity);
        const auto traded = trade(state, {orders, Orders{}}, rules);
        for (int k = 0; k < orders.count; ++k) {
            const auto order = orders.values[k];
            if ((order.op == kag::M_HIRE || order.op == kag::M_BUY_LAND) && traded.accepted[0][k])
                calendar.commitments.add(order.op);
        }
        const auto diagnostic = world.diagnose_solo_action(0, action, true);
        result.failed_work += diagnostic.requested_unit_actions - diagnostic.successful_unit_actions;
        consume(state, rules); apply(state.accounts[0], resources, calendar.after_market, rules.capacity);
        world.step(action, pass); ++result.simulated_turns;
        const auto after = kag::agent::runtime::make_observation(world, 0);
        if (!same_resources(state.accounts[0], resources, own_account(after), own_resources(after))) return result;
    }
    std::copy_n(world.st.farms[0].produced, kag::N_ITEMS, result.produced.begin());
    std::copy_n(world.st.farms[0].discarded, kag::N_ITEMS, result.discarded.begin());
    result.complete = true;
    // Hypothetical future weeds and nominal funding make this an estimated
    // calendar, not a verified executable schedule under actual market prices.
    result.plan.schedule_certified = false;
    return result;
}
}
