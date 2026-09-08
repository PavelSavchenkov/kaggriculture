#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_feed_p4_b4 {
class Agent:public compiler_feed_bundle_sep08::AgentCore {
public:
    Agent():AgentCore(4,true,true,true,1,true,0,0,1,4) {}
    static kag::agent::AgentInfo info() {return {"compiler_feed_p4_b4"};}
};
}
