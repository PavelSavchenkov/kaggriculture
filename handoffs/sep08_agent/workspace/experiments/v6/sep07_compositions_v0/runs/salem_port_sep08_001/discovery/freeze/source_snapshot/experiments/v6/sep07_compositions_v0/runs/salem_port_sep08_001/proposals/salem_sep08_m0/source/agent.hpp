#pragma once
#include "../../../source/policy.hpp"
namespace compositions::salem_sep08_m0 {
class Agent:public salem_port::Policy {
public:
    Agent():Policy(0){}
    static kag::agent::AgentInfo info(){return {"salem_sep08_m0"};}
};
}
