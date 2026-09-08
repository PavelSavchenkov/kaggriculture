#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/mixed/source/agent.hpp"
namespace compositions::joint_resources_mixed_m2 {
class Agent:public joint_resource_routes::Agent<joint_base_mixed::Agent,14,2> {
public:
    static kag::agent::AgentInfo info() {return {"joint_resources_mixed_m2"};}
};
}
