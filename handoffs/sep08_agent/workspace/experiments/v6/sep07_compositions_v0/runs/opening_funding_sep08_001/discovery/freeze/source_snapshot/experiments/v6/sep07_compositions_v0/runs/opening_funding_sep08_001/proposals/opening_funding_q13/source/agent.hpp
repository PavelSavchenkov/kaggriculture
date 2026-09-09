#pragma once
#include "../../../source/policy.hpp"
namespace compositions::opening_funding_q13 {
class Agent:public opening_funding::Policy {
public:
    Agent():Policy(13){}
    static kag::agent::AgentInfo info(){return {"opening_funding_q13"};}
};
}
