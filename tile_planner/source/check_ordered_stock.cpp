#include "fixed_finance.hpp"
#include <iostream>

int main() {
    using namespace placement;
    try {
        int checks = 0;
        for (int length = 1; length <= 6; ++length) for (int code = 0; code < (1 << (2 * length)); ++code) {
            Schedule orders;
            for (auto& action : orders) { action.n_units = 1; action.finalize(); }
            int bits = code, buys = 0, sales = 0, balance = 0, requirement = 0;
            const int hour = code % 24;
            orders[hour].n_orders = length;
            for (int s = 0; s < length; ++s) {
                const int value = bits & 3; bits >>= 2;
                const bool buy = value < 2; const int n = value % 2 + 1;
                orders[hour].orders[s] = {uint8_t(buy ? kag::M_BUY_PRODUCT : kag::M_SELL), kag::FERTILIZER, n};
                (buy ? buys : sales) += n;
                balance += buy ? n : -n; requirement = std::max(requirement, -balance);
            }
            orders[hour].finalize();
            DayProblem p;
            for (int h = hour; h < 24; ++h) p.shed_availability[h][kag::FERTILIZER] = std::max(0, sales - buys);
            int canceled = std::min(buys, sales);
            for (int s = 0; s < length; ++s) {
                const auto& order = orders[hour].orders[s];
                if (order.op != kag::M_BUY_PRODUCT) continue;
                const int removed = std::min(canceled, order.n); canceled -= removed;
                if (removed == order.n) continue;
                day_solver::MarketEvent event; event.hour = hour; event.order_index = s; event.market_op = order.op; event.item = order.item; event.quantity = order.n - removed;
                p.market_plan.push_back(event);
            }
            restore_ordered_stock(p, orders);
            int restored_buys = 0;
            for (const auto& event : p.market_plan) restored_buys += event.quantity;
            if (p.shed_availability[hour][kag::FERTILIZER] != requirement || restored_buys - requirement != buys - sales)
                throw std::runtime_error("ordered prefix or endpoint mismatch");
            for (int h = 0; h < 24; ++h) if (p.shed_availability[h][kag::FERTILIZER] != (h < hour ? 0 : requirement))
                throw std::runtime_error("wrong withdrawal hour");
            for (int stock = 0; stock <= 12; ++stock) {
                int available = stock; bool valid = true;
                for (int s = 0; s < length; ++s) {
                    const auto& order = orders[hour].orders[s];
                    available += order.op == kag::M_SELL ? -order.n : order.n;
                    valid &= available >= 0;
                }
                if (valid != (stock >= requirement)) throw std::runtime_error("prefix acceptance mismatch");
                ++checks;
            }
        }
        LifeDayContract contract; Day source;
        for (auto& values : source.problem.shed_availability) values[kag::CARROT] = 2;
        contract.day.problem.start.shed[kag::CARROT] = 1;
        bool rejected = false;
        try { apply_fixed_finance(contract, source, 7); }
        catch (const ResourceShortfall& error) { rejected = std::string(error.what()) == "insufficient_stock_day_7_item_1_missing_1"; }
        if (!rejected) throw std::runtime_error("missing stock was not reported before solving");
        ++checks;
        std::cout << checks << " ordered-stock checks passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
