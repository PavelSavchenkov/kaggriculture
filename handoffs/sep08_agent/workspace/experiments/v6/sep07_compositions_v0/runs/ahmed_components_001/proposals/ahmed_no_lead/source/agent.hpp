#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_no_lead {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(2){}
    static kag::agent::AgentInfo info(){return {"ahmed_no_lead"};}
};
}
