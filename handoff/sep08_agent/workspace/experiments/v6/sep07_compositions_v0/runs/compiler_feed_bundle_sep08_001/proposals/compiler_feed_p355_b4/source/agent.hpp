#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
#include "../../../../dated_expansion_sep08_001/proposals/dated_expansion_p355/source/agent.hpp"
namespace compositions::compiler_feed_p355_b4 {
class Agent:public compiler_feed_bundle_sep08::AgentCore {
public:
    Agent():AgentCore(std::vector<Life>(std::begin(dated_expansion_p355::lives),std::end(dated_expansion_p355::lives)),dated_expansion_p355::support,true,true,false,1,true,0,0,1,4) {}
    static kag::agent::AgentInfo info() {return {"compiler_feed_p355_b4"};}
};
}
