#pragma once
#include "portfolio.hpp"
#include "../native_constructor_generic_v2/constructor.hpp"

namespace day_native::portfolio::detail {
struct ResourceOptions {
    double seconds = 360, initial_exact_seconds = 5, initial_search_seconds = 180;
    double strong_route_seconds = 45, semantic_seconds = 40, screen_owner_seconds = 0;
    double route_seconds = 10, order_seconds = 60, exact_seconds = 20, cold_exact_seconds = 180;
    double early_seconds = 2;
    bool shared_seeds = false, defer_strong = true, dynamic_starts = false;
    int workers = 8, order_workers = 2;
    std::vector<int> route_seeds{3, 0, 2, 4}, order_seeds{0, 1, 2}, route_hours{22, 23, 24};
};

// Dependency injection is internal test infrastructure. Production solve()
// supplies native components and keeps one semantic cache for the whole day.
struct Backends {
    std::function<day_constructor::ConstructorResult(std::string_view, const day_solver::DayProblem&,
        day_constructor::ConstructorOptions)> construct;
    std::function<day_constructor::ConstructorResult(std::string_view, const day_solver::DayProblem&,
        day_constructor::ConstructorOptions, const std::vector<day_constructor::ProposalTask>&)> repair;
    std::function<semantic::Result(std::string_view, const InternalHint&, const semantic::Options&)> semantic;
    std::function<screen::Result(std::string_view, const day_solver::DayProblem&, const InternalHint&, const screen::SolveOptions&)> screen;
    std::function<exact::Result(std::string_view, const day_solver::DayProblem&, const exact::HintOptions&, const exact::SolveOptions&)> exact;
    std::function<double()> now;
};
bool needs_purchased_seeds(const day_solver::DayProblem&);
Result run(const day_solver::DayProblem&, const Options&, Backends = {},
           const std::optional<ResourceOptions>& resource_only = {});
} // namespace day_native::portfolio::detail
