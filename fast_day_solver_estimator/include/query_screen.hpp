#pragma once
#include "planning_context.hpp"

namespace labor {
struct QueryScreen {
    int requested_workers = 0;
    int capacity_bound = 1;
    int supply_bound = 1;
    int release_bound = 1;
    unsigned reasons = 0;
};

// Query context, not an absolute workforce predictor: the caller explicitly
// proposes this workforce and every hire time. All other slots have zero room.
// A nonzero mask is a necessary-condition failure, never a timeout inference.
inline QueryScreen screen_query(const day_solver::DayProblem& p, int hours) {
    std::array<std::pair<int, int>, 39> slots{}; int size = 0;
    for (const auto& event : p.market_plan) if (event.market_op == kag::M_HIRE) {
        if (size == 39) throw std::runtime_error("query workforce exceeds supported maximum");
        slots[size++] = {event.hour, event.order_index};
    }
    if (p.worker_count != size + 1) throw std::runtime_error("query workforce does not match committed hires");
    std::sort(slots.begin(), slots.begin() + size);
    const auto menu = fixed_planning_menu(p, hours, std::span(slots.data(), size), {});
    const auto f = extract(p, hours);
    const auto supply = supply_bound(p, menu.hires, hours);
    const auto release = release_bound(p, menu.hires, hours);
    QueryScreen result;
    result.requested_workers = p.worker_count;
    result.capacity_bound = workforce_lower_bound(p, f, menu.hires, hours);
    result.supply_bound = supply.workers; result.release_bound = release.workers;
    if (result.capacity_bound > p.worker_count) result.reasons |= 1;
    if (supply.workers > p.worker_count || supply.missing) result.reasons |= 2;
    if (release.workers > p.worker_count || release.seed_missing || release.land_missing) result.reasons |= 4;
    if (f[deadline_missing_quantity] > 0) result.reasons |= 8;
    return result;
}
}
