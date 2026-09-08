#pragma once
#include "../../../source/policy.hpp"
namespace catalog_cow_service_retained_q24_premium_m2_compositions::animal_groups_m2 {
class Agent:public animal_groups_policy::Policy {
public:
    Agent():Policy(2,-1,-1){}
    static kag::agent::AgentInfo info(){return {"animal_groups_m2"};}
};
}
