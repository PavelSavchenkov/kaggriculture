#pragma once
#include "../../public_capacity_router/source/agent.hpp"
namespace compositions::public_terminal_router {
class Agent:public public_capacity_router::AgentCore {
public:
    Agent():AgentCore(true) {}
    static kag::agent::AgentInfo info() {return {"public_terminal_router"};}
};
}
