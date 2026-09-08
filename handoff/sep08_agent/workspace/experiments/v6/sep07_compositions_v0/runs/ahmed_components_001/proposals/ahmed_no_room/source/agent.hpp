#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::ahmed_no_room {
class Agent:public compositions::ahmed_components::Agent {
public:
    Agent():compositions::ahmed_components::Agent(8){}
    static kag::agent::AgentInfo info(){return {"ahmed_no_room"};}
};
}
