#pragma once

#include "agents/common/api/agent_api.hpp"
#include "raw_agent.hpp"

namespace four_shop_search::external_replay_band_md_mehedi_hasan_89415485_safe {

class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

private:
    kag::agent::AgentConfig config_{};
    four_shop_search::external_replay_band_md_mehedi_hasan_89415485::Agent source_;
};

}
