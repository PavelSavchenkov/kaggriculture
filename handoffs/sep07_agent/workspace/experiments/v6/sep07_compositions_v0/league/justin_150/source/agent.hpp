#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::justin_150 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(150) {}
    static kag::agent::AgentInfo info() {return {"justin_150"};}
};
}
