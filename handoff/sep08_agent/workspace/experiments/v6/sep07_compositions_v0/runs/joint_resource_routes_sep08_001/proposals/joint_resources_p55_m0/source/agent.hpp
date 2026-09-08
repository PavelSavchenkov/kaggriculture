#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/p55/source/agent.hpp"
namespace compositions::joint_resources_p55_m0 {
class Agent:public joint_resource_routes::Agent<joint_base_p55::Agent,14,0> {
public:
    static kag::agent::AgentInfo info() {return {"joint_resources_p55_m0"};}
};
}
