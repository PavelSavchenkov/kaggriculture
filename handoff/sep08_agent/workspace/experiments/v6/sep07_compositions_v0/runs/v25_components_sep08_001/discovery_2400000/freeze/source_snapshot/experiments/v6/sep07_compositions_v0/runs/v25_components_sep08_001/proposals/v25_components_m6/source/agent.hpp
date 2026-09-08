#pragma once
#include "../../../source/policy.hpp"
namespace compositions::v25_components_m6 {
class Agent:public v25_components::Agent {
public:
    Agent():v25_components::Agent(6){}
    static kag::agent::AgentInfo info(){return {"v25_components_m6"};}
};
}
