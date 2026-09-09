#pragma once
#include "planning_features.hpp"

namespace labor {
// Fixed commitments are selected before optional entries, even when the latter
// occur earlier in time. This is selection order, not actual worker numbering.
// Capacity bounds need only the selected multiset of worker birth times.
inline PlanningMenu fixed_planning_menu(const day_solver::DayProblem& p, int hours,
        std::span<const std::pair<int, int>> fixed, std::span<const std::pair<int, int>> optional) {
    if (fixed.size() + optional.size() > 39) throw std::runtime_error("too many total hire slots");
    const auto checked_fixed = planning_menu(p, hours, fixed, true);
    const auto checked_optional = planning_menu(p, hours, optional);
    auto result = checked_fixed; result.minimum_workers = fixed.size() + 1;
    std::array<std::array<bool, 10>, 24> occupied{};
    for (const auto& [hour, slot] : fixed) occupied[hour][slot] = true;
    for (int i = 0; i < checked_optional.size; ++i) {
        const int hour = checked_optional.hires.hours[i], slot = checked_optional.hires.slots[i];
        if (occupied[hour][slot]) throw std::runtime_error("optional hire overlaps fixed commitment");
        result.hires.hours[result.size] = hour; result.hires.slots[result.size] = slot; ++result.size;
    }
    return result;
}

using ContextFeatures = std::array<float, planning_count + 1>;

inline ContextFeatures extract_context(const day_solver::DayProblem& p, const PlanningMenu& menu) {
    ContextFeatures result{};
    const auto planning = extract_planning(p, menu);
    std::copy(planning.begin(), planning.end(), result.begin());
    result.back() = menu.minimum_workers;
    return result;
}

inline std::vector<std::string> context_feature_names() {
    auto names = planning_feature_names();
    names[planning_slots] = "available_hire_slots";
    for (int i = 0; i < 39; ++i) names[planning_birth_base + i] = "selection_birth_" + std::to_string(i);
    names.push_back("committed_workforce"); return names;
}
}
