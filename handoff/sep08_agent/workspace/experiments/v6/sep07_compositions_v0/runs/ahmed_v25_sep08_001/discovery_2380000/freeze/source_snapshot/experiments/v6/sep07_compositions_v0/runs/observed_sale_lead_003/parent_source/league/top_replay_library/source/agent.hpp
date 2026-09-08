#pragma once
#include "../../../../../../../../../agents/common/api/agent_api.hpp"

namespace compositions_sale_copy::top_replay_library {
int program_count();
class Agent {
public:
    explicit Agent(int program=0);
    static kag::agent::AgentInfo info() {return {"top_replay_library"};}
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
private:
    int program_=0;
};
}
