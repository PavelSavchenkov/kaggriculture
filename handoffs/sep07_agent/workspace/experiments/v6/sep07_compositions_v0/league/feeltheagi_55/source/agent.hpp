#pragma once
#include "../../top_replay_library/source/agent.hpp"

namespace compositions::feeltheagi_55 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(55) {}
    static kag::agent::AgentInfo info() {return {"feeltheagi_55"};}
};
}
