#pragma once
#include "lifetimes.hpp"

namespace placement {
struct ResourceShortfall : std::runtime_error {
    ResourceShortfall(int day, int item, int64_t missing, bool seed)
        : std::runtime_error(std::string(seed ? "insufficient_seeds_day_" : "insufficient_stock_day_") + std::to_string(day) + "_item_" + std::to_string(item) + "_missing_" + std::to_string(missing)) {}
};
// The physical scheduler withdraws before buying. Netting every round trip
// loses a sell-before-buy requirement. Restore the maximum ordered prefix
// deficit and an equal amount of that hour's canceled purchases. No worker
// acts between order slots, so this is the exact stock requirement without
// modeling prices, money, or capacity.
inline void restore_ordered_stock(DayProblem& p, const Schedule& orders) {
    const auto original_withdrawals = p.shed_availability;
    for (int h = 0; h < 24; ++h) {
        std::array<int64_t, kag::N_ITEMS> deficit{}, required{};
        for (int s = 0; s < orders[h].n_orders; ++s) {
            const auto& order = orders[h].orders[s];
            if (order.op == kag::M_SELL) deficit[order.item] += order.n;
            if (order.op == kag::M_BUY_PRODUCT) deficit[order.item] -= order.n;
            if (order.op == kag::M_SELL || order.op == kag::M_BUY_PRODUCT)
                required[order.item] = std::max(required[order.item], deficit[order.item]);
        }
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            const auto withdrawn = original_withdrawals[h][item] - (h ? original_withdrawals[h - 1][item] : 0);
            int64_t extra = std::max<int64_t>(0, required[item] - withdrawn);
            for (int t = h; t < 24; ++t) p.shed_availability[t][item] += extra;
            for (int s = 0; s < orders[h].n_orders && extra; ++s) {
                const auto& order = orders[h].orders[s];
                if (order.op != kag::M_BUY_PRODUCT || order.item != item) continue;
                auto event = std::find_if(p.market_plan.begin(), p.market_plan.end(), [&](const auto& e) { return e.hour == h && e.order_index == s; });
                const int64_t present = event == p.market_plan.end() ? 0 : event->quantity;
                if (present > order.n) throw std::runtime_error("fixed purchase exceeds its executable order");
                const auto restored = std::min<int64_t>(extra, order.n - present);
                if (!restored) continue;
                if (event == p.market_plan.end()) {
                    day_solver::MarketEvent added; added.hour = h; added.order_index = s; added.market_op = order.op; added.item = item; added.quantity = restored;
                    p.market_plan.push_back(added);
                } else event->quantity += restored;
                extra -= restored;
            }
            if (extra) throw std::runtime_error("ordered stock requirement has no matching canceled purchase");
        }
    }
    std::sort(p.market_plan.begin(), p.market_plan.end(), [](const auto& a, const auto& b) { return std::pair{a.hour, a.order_index} < std::pair{b.hour, b.order_index}; });
}

// The outer planner owns these dated purchases and withdrawals. Preserve
// every accepted trade in the executable orders, including round trips.
inline void apply_fixed_finance(LifeDayContract& contract, const Day& source, int d) {
    auto& day = contract.day; auto& p = day.problem;
    p.market_plan = source.problem.market_plan;
    std::erase_if(p.market_plan, [](const auto& event) { return event.market_op == kag::M_HIRE; });
    p.shed_availability = source.problem.shed_availability;
    restore_ordered_stock(p, source.executable);
    p.sale_targets.clear(); p.allowed_acquisitions.clear(); p.required_outcomes.clear();
    p.end_shed = p.start.shed; p.end_seeds = p.start.seeds;
    for (const auto& event : p.market_plan) {
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL) p.end_shed[event.item] += event.quantity;
        else if (event.market_op == kag::M_BUY_SEED) p.end_seeds[event.item] += event.quantity;
        else if (event.market_op != kag::M_BUY_LAND) throw std::runtime_error("unsupported fixed finance event");
    }
    for (const auto& tile : p.tile_work) for (const auto& action : tile.actions) {
        if (action.op == kag::OP_PLANT) --p.end_seeds[action.arg];
        if (action.op == kag::OP_FEED) --p.end_shed[kag::WHEAT];
        if (action.op == kag::OP_FERTILIZE) --p.end_shed[kag::FERTILIZER];
        if (action.op == kag::OP_PLACE) --p.end_shed[action.arg];
        if (action.output_item >= 0) p.end_shed[action.output_item] += action.output_quantity;
    }
    for (int item = 0; item < kag::N_ITEMS; ++item) p.end_shed[item] -= p.shed_availability[23][item];
    for (int item = 0; item < kag::N_ITEMS; ++item) if (p.end_shed[item] < 0) throw ResourceShortfall(d, item, -p.end_shed[item], false);
    for (int item = 0; item < kag::N_CROPS; ++item) if (p.end_seeds[item] < 0) throw ResourceShortfall(d, item, -p.end_seeds[item], true);
    for (int h = 0; h < 24; ++h) {
        day.executable[h] = source.executable[h];
        std::fill(std::begin(day.executable[h].units), std::end(day.executable[h].units), kag::UnitAction{});
        day.executable[h].n_units = 1;
        for (auto& order : day.executable[h].orders) if (order.op == kag::M_HIRE) order = {};
        day.executable[h].finalize();
    }
    day_scheduler::prepare_problem(p);
    if (d == 29) labor::offline::require_terminal_work(p);
    day.menu = legal_menu(day, d == 29 ? 23 : 24);
}
}
