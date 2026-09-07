#pragma once
#include "../native_solver_api/internal_hint.hpp"
#include "day_solver_api.hpp"
#include <boost/multiprecision/cpp_int.hpp>

namespace day_native::quick_v30 {
// Test necessary purchased-stock conditions on OUR coarse task times. Items
// produced during the day are excluded here so route-local reuse stays legal.
inline bool violates_purchased_stock(const day_solver::DayProblem& problem, const InternalHint& hint) {
    using Wide = boost::multiprecision::int128_t;
    std::array<bool, kag::N_ITEMS> produced{};
    std::vector<int> input;
    for (const auto& work : problem.tile_work) for (const auto& task : work.actions) {
        if (task.output_item >= 0 && task.output_quantity > 0) produced[task.output_item] = true;
        input.push_back(task.op == kag::OP_FEED ? kag::WHEAT : task.op == kag::OP_FERTILIZE ? kag::FERTILIZER :
                        task.op == kag::OP_PLACE ? task.arg : -1);
    }
    std::array<std::array<int, day_solver::HOURS>, kag::N_ITEMS> consumed{};
    std::array<int, kag::N_ITEMS> first_buy;
    first_buy.fill(day_solver::HOURS);
    for (const auto& event : problem.market_plan)
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)
            first_buy[event.item] = std::min(first_buy[event.item], int(event.hour));
    for (const auto& task : hint.assignments) {
        const int item = input.at(task.task), hour = required(task.hour, "hour");
        if (item < 0 || produced[item]) continue;
        if (!problem.start.shed[item] && hour < first_buy[item] + 2) return true;
        ++consumed[item].at(hour);
    }
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        if (produced[item]) continue;
        Wide available = problem.start.shed[item];
        int used = 0;
        for (int hour = 0; hour < day_solver::HOURS; ++hour) {
            used += consumed[item][hour];
            if (Wide(used) + problem.shed_availability[hour][item] > available) return true;
            for (const auto& event : problem.market_plan)
                if (event.hour == hour && event.item == item &&
                    (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)) available += event.quantity;
        }
    }
    return false;
}
}
