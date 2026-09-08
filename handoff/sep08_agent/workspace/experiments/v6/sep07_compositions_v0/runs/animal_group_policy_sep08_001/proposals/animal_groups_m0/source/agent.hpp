#pragma once
#include "../../../source/policy.hpp"
namespace compositions::animal_groups_m0 {
class Agent:public animal_groups_policy::Policy {
public:
    Agent():Policy(0,-1,-1){}
    static kag::agent::AgentInfo info(){return {"animal_groups_m0"};}
};
}
