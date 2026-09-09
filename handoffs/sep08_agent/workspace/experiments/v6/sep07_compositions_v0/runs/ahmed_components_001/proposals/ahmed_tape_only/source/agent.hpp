#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_tape_only {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(127){}
    static kag::agent::AgentInfo info(){return {"ahmed_tape_only"};}
};
}
