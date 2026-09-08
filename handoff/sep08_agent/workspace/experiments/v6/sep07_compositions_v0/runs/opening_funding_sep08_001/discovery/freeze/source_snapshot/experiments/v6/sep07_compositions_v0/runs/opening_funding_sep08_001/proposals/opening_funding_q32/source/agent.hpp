#pragma once
#include "../../../source/policy.hpp"
namespace compositions::opening_funding_q32 {
class Agent:public opening_funding::Policy {
public:
    Agent():Policy(32){}
    static kag::agent::AgentInfo info(){return {"opening_funding_q32"};}
};
}
