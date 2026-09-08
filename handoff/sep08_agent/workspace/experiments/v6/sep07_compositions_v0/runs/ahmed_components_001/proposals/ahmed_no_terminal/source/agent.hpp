#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_no_terminal {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(64){}
    static kag::agent::AgentInfo info(){return {"ahmed_no_terminal"};}
};
}
