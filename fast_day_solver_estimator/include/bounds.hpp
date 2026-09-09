#pragma once
#include "features.hpp"

namespace labor {
struct HireMenu {
    std::array<int, 39> hours{};
    std::array<int, 39> slots{};
};

inline HireMenu earliest_menu(const day_solver::DayProblem& problem, int active_hours = 24) {
    if (active_hours != 23 && active_hours != 24) throw std::runtime_error("unsupported active horizon");
    std::array<std::array<bool, 10>, 23> occupied{};
    for (const auto& e : problem.market_plan)
        if (e.market_op != kag::M_HIRE && e.hour < 23) occupied[e.hour][e.order_index] = true;
    HireMenu menu; int n = 0;
    for (int h = 0; h < active_hours - 1 && n < 39; ++h) for (int s = 0; s < 10 && n < 39; ++s)
        if (!occupied[h][s]) { menu.hours[n] = h; menu.slots[n] = s; ++n; }
    if (n != 39) throw std::runtime_error("fewer than 39 allowed hiring slots");
    return menu;
}

// Necessary action-capacity conditions; inventory/order/botanical constraints
// are relaxed. Returning k proves only that smaller workforces cannot suffice
// under this menu. It does not prove feasibility at k.
inline int workforce_lower_bound(const day_solver::DayProblem& problem, const Features& f, const HireMenu& menu, int active_hours = 24) {
    std::array<int, 11> far_tasks{};
    for (const auto& work : problem.tile_work) {
        const auto& tile = problem.start.managed_tiles[work.tile];
        const int d = shed_distance(tile.y * 10 + tile.x);
        for (int r = 0; r <= d; ++r) far_tasks[r] += work.actions.size();
    }
    for (int workers = 1; workers <= 40; ++workers) {
        int capacity = active_hours;
        for (int u = 0; u < workers - 1; ++u) capacity += active_hours - 1 - menu.hours[u];
        if (capacity < f[task_motion_bound]) continue;
        bool possible = true;
        for (int r = 0; r <= 10; ++r) {
            int room = std::max(0, active_hours - r);
            for (int u = 0; u < workers - 1; ++u) room += std::max(0, active_hours - 1 - menu.hours[u] - r);
            if (room < far_tasks[r]) { possible = false; break; }
        }
        for (int h = 0; h < active_hours && possible; ++h) {
            int room = h + 1;
            for (int u = 0; u < workers - 1; ++u) room += std::max(0, h - menu.hours[u]);
            if (room < f[deadline_required_actions_base + h]) possible = false;
        }
        if (possible) return workers;
    }
    return 41;
}

inline int64_t hire_cost(int workers) {
    if (workers < 1 || workers > 40) throw std::runtime_error("workforce outside 1..40");
    int64_t a = 1, b = 1, total = 0;
    for (int u = 1; u < workers; ++u) { total += a; const auto next = a + b; a = b; b = next; }
    return total;
}
}
