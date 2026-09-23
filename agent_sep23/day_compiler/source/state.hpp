#pragma once
#include "agents/common/api/agent_api.hpp"

namespace kag::day_compiler {
using Observation = agent::AgentObservation;
using Configuration = agent::AgentConfig;

int distance(int first, int second);
int shed_distance(int cell);
int demand(const Observation& observation, const Configuration& config, int product, int step);
void rebuild_masks(Farm& farm);
Farm own_farm(const Observation& observation);
Sim observed_state(const Observation& observation, const Configuration& config = {});
Farm worker_phase(const Observation& observation, const Action& action, const Configuration& config = {});
bool same_entity(const Tile& first, const Tile& second);
}
