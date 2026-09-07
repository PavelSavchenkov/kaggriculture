#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::justin_154 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(154) {}
    static kag::agent::AgentInfo info() {return {"justin_154"};}
};
}
