#pragma once
#include "planning_context.hpp"

namespace labor {
constexpr int context_query_count = count + 2 + 24 + 9 + 24 + 20 + 8;
using ContextQueryFeatures = std::array<double, context_query_count>;

// Only the selected query is described. Unselected optional slots, total menu
// size and the mandatory/optional distinction do not change a cold solver call.
inline ContextQueryFeatures context_query_features(const ContextFeatures& f, int workers) {
    const int hours = int(f[planning_hours]), available = int(f[planning_slots]);
    if ((hours != 23 && hours != 24) || workers < int(f.back()) || workers > available + 1)
        throw std::runtime_error("invalid context query");
    ContextQueryFeatures result{}; int at = 0;
    for (int i = 0; i < count; ++i) result[at++] = f[i];
    result[at++] = hours; result[at++] = workers;
    std::array<double, 24> prefix{};
    for (int h = 0; h < 24; ++h) {
        const int end = std::min(h + 1, hours);
        double capacity = end;
        for (int i = 0; i < workers - 1; ++i) capacity += std::max(0, end - int(f[planning_birth_base + i]));
        prefix[h] = capacity; result[at++] = capacity;
    }
    for (int radius = 0; radius <= 8; ++radius) {
        double capacity = std::max(0, hours - radius);
        for (int i = 0; i < workers - 1; ++i) capacity += std::max(0, hours - int(f[planning_birth_base + i]) - radius);
        result[at++] = capacity;
    }
    for (int h = 0; h < 24; ++h) result[at++] = f[deadline_required_actions_base + h] / std::max(1.0, prefix[h]);
    const int selected[] = {tasks, task_motion_bound, task_distance, rooted_mst, input_actions, output_actions,
                            route_pack_open, route_pack_return, deadline_pressure_max, required_pickup_types};
    for (int feature : selected) {
        result[at++] = double(f[feature]) / workers;
        result[at++] = f[feature] / std::max(1.0, prefix[23]);
    }
    // These descriptors depend only on physical work and purchase releases.
    // Omit lower bounds and tight-cut locations, which can depend on the menu.
    for (int feature : {planning_supply_base + 1, planning_supply_base + 2, planning_supply_base + 5,
                        planning_release_base + 1, planning_release_base + 2, planning_release_base + 3,
                        planning_release_base + 4, planning_release_base + 7}) result[at++] = f[feature];
    if (at != context_query_count) std::abort();
    return result;
}
}
