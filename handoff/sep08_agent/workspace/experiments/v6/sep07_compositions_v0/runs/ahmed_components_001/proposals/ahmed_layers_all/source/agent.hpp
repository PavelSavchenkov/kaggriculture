#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_layers_all {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(0){}
    static kag::agent::AgentInfo info(){return {"ahmed_layers_all"};}
};
}
