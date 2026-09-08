#pragma once
#include "../../teammate_sixday/source/agent.hpp"
namespace catalog_public_sixday_compositions::public_sixday {
class Agent:public teammate_sixday::AgentCore {
public:
Agent():AgentCore(0) {}
static kag::agent::AgentInfo info() {return {"public_sixday"};}
};
}
