#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::mao_89 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(89) {}
    static kag::agent::AgentInfo info() {return {"mao_89"};}
};
}
