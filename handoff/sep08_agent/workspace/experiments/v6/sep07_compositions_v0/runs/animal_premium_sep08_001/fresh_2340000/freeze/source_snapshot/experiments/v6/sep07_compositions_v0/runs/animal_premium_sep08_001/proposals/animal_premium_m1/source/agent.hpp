#pragma once
#include "experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/proposals/animal_groups_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"
namespace compositions::animal_premium_m1 {
class Agent:public premium_sales::Policy<compositions::animal_groups_m1::Agent,216> {
public:
    static kag::agent::AgentInfo info(){return {"animal_premium_m1"};}
};
}
