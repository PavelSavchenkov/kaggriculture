#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/p4/source/agent.hpp"
namespace compositions::joint_resources_p4_m7 {
class Agent:public joint_resource_routes::Agent<joint_base_p4::Agent,14,7> {
public:
    static kag::agent::AgentInfo info() {return {"joint_resources_p4_m7"};}
};
}
