#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/p355/source/agent.hpp"
namespace compositions::joint_resources_p355_m1 {
class Agent:public joint_resource_routes::Agent<joint_base_p355::Agent,14,1> {
public:
    static kag::agent::AgentInfo info() {return {"joint_resources_p355_m1"};}
};
}
