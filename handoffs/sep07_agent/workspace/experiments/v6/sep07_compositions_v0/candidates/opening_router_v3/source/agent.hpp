#pragma once
#include "../../hire_day1_day9/source/agent.hpp"
namespace compositions::opening_router_v3 {
class Agent:public hire_day1_day9::Agent {
public:
    static kag::agent::AgentInfo info() {return {"opening_router_v3"};}
};
}
