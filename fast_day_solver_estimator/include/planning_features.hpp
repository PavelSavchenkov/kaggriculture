#pragma once
#include "release_bounds.hpp"
#include "supply_bounds.hpp"
#include <span>

namespace labor {
struct PlanningMenu {
    HireMenu hires;
    int hours = 24;
    int size = 0;
    int minimum_workers = 1;
};

// Explicit caller-owned optional slots. They are not inferred from the
// workforce or hire events in a source certificate.
inline PlanningMenu planning_menu(const day_solver::DayProblem& p, int hours, std::span<const std::pair<int, int>> slots, bool allow_inactive_hires = false) {
    if ((hours != 23 && hours != 24) || slots.size() > 39) throw std::runtime_error("invalid planning menu size/horizon");
    PlanningMenu result; result.hours = hours; result.size = slots.size();
    result.hires.hours.fill(hours - 1); result.hires.slots.fill(-1);
    std::array<std::array<bool, 10>, 24> occupied{};
    for (const auto& e : p.market_plan) if (e.market_op != kag::M_HIRE) occupied[e.hour][e.order_index] = true;
    std::pair<int, int> previous{-1, -1};
    for (int i = 0; i < result.size; ++i) {
        const auto [hour, slot] = slots[i];
        if (hour < 0 || hour >= hours - (allow_inactive_hires ? 0 : 1) || slot < 0 || slot >= 10 || slots[i] <= previous || occupied[hour][slot])
            throw std::runtime_error("invalid, unordered, or occupied optional hire slot");
        result.hires.hours[i] = hour; result.hires.slots[i] = slot; previous = slots[i];
    }
    return result;
}

inline int planning_capacity(const PlanningMenu& menu, int workers, int through_hour = -1, int radius = 0) {
    if (workers < menu.minimum_workers || workers > menu.size + 1 || radius < 0) throw std::runtime_error("invalid requested workforce/radius");
    const int end = through_hour < 0 ? menu.hours : through_hour + 1;
    if (end > menu.hours) throw std::runtime_error("capacity after active horizon");
    int capacity = std::max(0, end - radius);
    for (int i = 0; i < workers - 1; ++i) capacity += std::max(0, end - menu.hires.hours[i] - 1 - radius);
    return capacity;
}

enum PlanningFeature {
    planning_hours = count,
    planning_slots,
    planning_birth_base,
    planning_lower = planning_birth_base + 39,
    planning_supply_base,
    planning_release_base = planning_supply_base + 6,
    planning_count = planning_release_base + 8
};
using PlanningFeatures = std::array<float, planning_count>;

inline PlanningFeatures extract_planning(const day_solver::DayProblem& p, const PlanningMenu& menu) {
    PlanningFeatures result{};
    const auto base = extract(p, menu.hours);
    std::copy(base.begin(), base.end(), result.begin());
    result[planning_hours] = menu.hours; result[planning_slots] = menu.size;
    for (int i = 0; i < 39; ++i) result[planning_birth_base + i] = menu.hires.hours[i] + 1;
    const auto supply = supply_bound(p, menu.hires, menu.hours);
    const auto release = release_bound(p, menu.hires, menu.hours);
    result[planning_lower] = std::max({menu.minimum_workers, workforce_lower_bound(p, base, menu.hires, menu.hours), supply.workers, release.workers});
    const std::array<float, 6> carried{float(supply.workers), float(supply.missing), float(supply.late_actions), float(supply.tight_release), float(supply.tight_radius), supply.pressure};
    const std::array<float, 8> global{float(release.workers), float(release.seed_missing), float(release.land_missing), float(release.seed_late_actions), float(release.land_late_actions), float(release.tight_release), float(release.tight_radius), release.pressure};
    std::copy(carried.begin(), carried.end(), result.begin() + planning_supply_base);
    std::copy(global.begin(), global.end(), result.begin() + planning_release_base);
    return result;
}

inline std::vector<std::string> planning_feature_names() {
    auto names = feature_names();
    names.push_back("active_hours"); names.push_back("optional_hire_slots");
    for (int i = 0; i < 39; ++i) names.push_back("optional_birth_" + std::to_string(i));
    for (const auto& name : {"planning_lower_bound", "supply_lower_bound", "supply_missing", "supply_late_actions", "supply_tight_release", "supply_tight_radius", "supply_pressure",
                            "release_lower_bound", "seed_missing", "land_missing", "seed_late_actions", "land_late_actions", "release_tight_hour", "release_tight_radius", "release_pressure"}) names.emplace_back(name);
    if (names.size() != planning_count) std::abort();
    return names;
}
}
