#pragma once
#include "../../../routes.hpp"
#include "../../../bases/mixed/source/agent.hpp"
namespace compositions::joint_routes_mixed_m1 {
class Agent:public joint_day_routes::Agent<joint_base_mixed::Agent,14> {
public:
    static kag::agent::AgentInfo info() {return {"joint_routes_mixed_m1"};}
};
}
