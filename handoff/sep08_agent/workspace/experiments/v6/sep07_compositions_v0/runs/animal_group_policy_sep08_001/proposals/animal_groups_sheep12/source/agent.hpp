#pragma once
#include "../../../source/policy.hpp"
namespace compositions::animal_groups_sheep12 {
class Agent:public animal_groups_policy::Policy {
public:
    Agent():Policy(1,1,2){}
    static kag::agent::AgentInfo info(){return {"animal_groups_sheep12"};}
};
}
