#pragma once
#include "../../teammate_sixday/source/agent.hpp"
namespace compositions::teammate_sixday_robust {
class Agent:public teammate_sixday::AgentCore {
public:
Agent():AgentCore(2) {}
static kag::agent::AgentInfo info() {return {"teammate_sixday_robust"};}
};
}
