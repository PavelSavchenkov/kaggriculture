#pragma once
#include "../../../source/policy.hpp"
namespace compositions::opening_funding_q20 {
class Agent:public opening_funding::Policy {
public:
    Agent():Policy(20){}
    static kag::agent::AgentInfo info(){return {"opening_funding_q20"};}
};
}
