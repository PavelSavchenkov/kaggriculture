#pragma once
#include "../../../../../../../common/api/agent_api.hpp"

namespace catalog_crop_mix_t2_wheat_compositions::public_router {
class Agent {
public:
    static kag::agent::AgentInfo info() { return {"public_router"}; }
    void reset(const kag::agent::AgentInit&) { route_ = 0; }
    const kag::Action& planned_action(int step) const;
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget&, kag::Action& action);
private:
    int route_ = 0;
};
}
