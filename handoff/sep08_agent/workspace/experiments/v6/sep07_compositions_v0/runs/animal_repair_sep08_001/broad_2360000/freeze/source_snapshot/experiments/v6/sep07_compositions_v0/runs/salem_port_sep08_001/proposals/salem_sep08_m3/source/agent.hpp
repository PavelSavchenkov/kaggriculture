#pragma once
#include "../../../source/policy.hpp"
namespace compositions::salem_sep08_m3 {
class Agent:public salem_port::Policy {
public:
    Agent():Policy(3){}
    static kag::agent::AgentInfo info(){return {"salem_sep08_m3"};}
};
}
