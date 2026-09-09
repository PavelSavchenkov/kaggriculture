#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/goose/source/agent.hpp"
namespace compositions::joint_resources_goose_m7 {
class Agent:public joint_resource_routes::Agent<joint_base_goose::Agent,14,7> {
public:
    static kag::agent::AgentInfo info() {return {"joint_resources_goose_m7"};}
};
}
