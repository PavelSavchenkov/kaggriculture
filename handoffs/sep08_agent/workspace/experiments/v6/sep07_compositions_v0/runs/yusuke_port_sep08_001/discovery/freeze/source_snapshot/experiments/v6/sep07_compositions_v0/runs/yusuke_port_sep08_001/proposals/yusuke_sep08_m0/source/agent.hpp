#pragma once
#include "../../../source/policy.hpp"
namespace compositions::yusuke_sep08_m0 {
class Agent:public yusuke_port::Policy {
public:
    Agent():Policy(0){}
    static kag::agent::AgentInfo info(){return {"yusuke_sep08_m0"};}
};
}
