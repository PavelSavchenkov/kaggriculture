#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_no_budget {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(4){}
    static kag::agent::AgentInfo info(){return {"ahmed_no_budget"};}
};
}
