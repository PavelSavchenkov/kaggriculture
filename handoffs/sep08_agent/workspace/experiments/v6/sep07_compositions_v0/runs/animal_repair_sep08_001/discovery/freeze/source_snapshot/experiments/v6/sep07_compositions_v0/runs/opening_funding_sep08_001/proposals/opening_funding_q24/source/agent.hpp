#pragma once
#include "../../../source/policy.hpp"
namespace compositions::opening_funding_q24 {
class Agent:public opening_funding::Policy {
public:
    Agent():Policy(24){}
    static kag::agent::AgentInfo info(){return {"opening_funding_q24"};}
};
}
