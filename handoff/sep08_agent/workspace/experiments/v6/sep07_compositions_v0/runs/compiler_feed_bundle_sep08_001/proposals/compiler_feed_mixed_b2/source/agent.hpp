#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::compiler_feed_mixed_b2 {
class Agent:public compiler_feed_bundle_sep08::AgentCore {
public:
    Agent():AgentCore(compiler_labor_data::cold_farm(2,2,0,7,12,8),Support{},false,false,false,1,true,1,1,1,2) {}
    static kag::agent::AgentInfo info() {return {"compiler_feed_mixed_b2"};}
};
}
