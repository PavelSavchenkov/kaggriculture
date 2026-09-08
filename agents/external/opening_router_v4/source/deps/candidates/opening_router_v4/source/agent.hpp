#pragma once
#include "../../guarded_hires_v3/source/agent.hpp"
namespace catalog_opening_router_v4_compositions::opening_router_v4 {
class Agent:public guarded_hires_v3::Agent {
public:
    static kag::agent::AgentInfo info() {return {"opening_router_v4"};}
};
}
