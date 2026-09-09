#pragma once
#include "../../../routes.hpp"
#include "../../../bases/p55/source/agent.hpp"
namespace compositions::joint_routes_p55_m2 {
class Agent:public joint_day_routes::Agent<joint_base_p55::Agent,0> {
public:
    static kag::agent::AgentInfo info() {return {"joint_routes_p55_m2"};}
};
}
