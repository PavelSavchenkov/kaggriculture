#include "agent.hpp"

#include "observation_sim.hpp"
#include "optimized_active_safety.hpp"

namespace four_shop_search::external_replay_band_yaroslav_tanko_89416002_safe {

kag::agent::AgentInfo Agent::info() { return {"external_replay_band_yaroslav_tanko_89416002_safe"}; }

void Agent::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    source_.reset(init);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    source_.act(observation, budget, action);
    const kag::Sim sim = four_shop_foundry::observation_sim::make_sim(
        config_, observation);
    four_shop_foundry::optimized_active_safety::sanitize(
        sim, config_, observation, action);
}

}
