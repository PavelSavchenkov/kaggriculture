#pragma once
#include "../../../../../candidates/composition_greedy_v0/source/agent.hpp"
namespace compositions::search_v0_383 {
#include "plan.inc"
class Agent:public greedy::AgentCore {
public:
Agent():greedy::AgentCore(std::vector<Life>(lives,lives+sizeof(lives)/sizeof(lives[0])),support,true,false,false,1,true) {}
static kag::agent::AgentInfo info(){return {"search_v0_383"};}
};
}
