#pragma once
#include "../../../routes.hpp"
#include "../../../bases/p355/source/agent.hpp"
namespace compositions::joint_routes_p355_m1 {
class Agent:public joint_day_routes::Agent<joint_base_p355::Agent,14> {
public:
    static kag::agent::AgentInfo info() {return {"joint_routes_p355_m1"};}
};
}
