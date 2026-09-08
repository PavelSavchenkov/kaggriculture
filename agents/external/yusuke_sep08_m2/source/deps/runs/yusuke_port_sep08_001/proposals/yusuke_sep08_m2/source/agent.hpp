#pragma once
#include "../../../source/policy.hpp"
namespace catalog_yusuke_sep08_m2_compositions::yusuke_sep08_m2 {
class Agent:public yusuke_port::Policy {
public:
    Agent():Policy(2){}
    static kag::agent::AgentInfo info(){return {"yusuke_sep08_m2"};}
};
}
