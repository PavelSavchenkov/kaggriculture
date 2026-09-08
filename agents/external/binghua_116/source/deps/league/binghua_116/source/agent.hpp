#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace catalog_binghua_116_compositions::binghua_116 {
class Agent:public top_replay_library::Agent {
public:
    Agent():top_replay_library::Agent(116) {}
    static kag::agent::AgentInfo info() {return {"binghua_116"};}
};
}
