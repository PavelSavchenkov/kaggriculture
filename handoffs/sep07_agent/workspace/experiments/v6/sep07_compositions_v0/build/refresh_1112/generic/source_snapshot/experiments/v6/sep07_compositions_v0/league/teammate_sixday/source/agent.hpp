#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::teammate_sixday {
class AgentCore {
public:
    explicit AgentCore(int mode=1):mode_(mode) {}
    static kag::agent::AgentInfo info() {return {"teammate_sixday"};}
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget,kag::Action& action);
private:
    kag::Config config_{};
    int mode_;
    int selected_[5]{};
};
using Agent=AgentCore;
}
