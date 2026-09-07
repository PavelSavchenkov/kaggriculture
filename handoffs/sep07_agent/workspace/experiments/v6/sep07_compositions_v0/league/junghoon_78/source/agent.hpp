#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::junghoon_78 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(78) {}
    static kag::agent::AgentInfo info() {return {"junghoon_78"};}
};
}
