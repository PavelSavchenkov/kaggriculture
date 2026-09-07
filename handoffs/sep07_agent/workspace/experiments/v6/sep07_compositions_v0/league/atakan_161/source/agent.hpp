#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::atakan_161 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(161) {}
    static kag::agent::AgentInfo info() {return {"atakan_161"};}
};
}
