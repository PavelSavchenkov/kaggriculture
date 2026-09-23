#pragma once
#include "agent.hpp"

namespace kag::agents::sep22_worker {
// Small deterministic vehicle-routing heuristic over complete service visits.
// Only own observed state and the supplied day plan enter the constructor.
void add_deliveries(const agent::AgentObservation& observation, DayPlan& plan, const DayReturns* returns = nullptr, int source_seed = 0, bool reachable_sources = true);
Routes construct_routes(const agent::AgentObservation& observation, DayPlan& plan,
                        int rounds, int variant, int auto_hires = -1, const agent::DecisionBudget& budget = {});
}
