#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::john_131 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(131) {}
    static kag::agent::AgentInfo info() {return {"john_131"};}
};
}
