#pragma once
#include "../../../source/policy.hpp"
namespace compositions::yusuke_sep08_m3 {
class Agent:public yusuke_port::Policy {
public:
    Agent():Policy(3){}
    static kag::agent::AgentInfo info(){return {"yusuke_sep08_m3"};}
};
}
