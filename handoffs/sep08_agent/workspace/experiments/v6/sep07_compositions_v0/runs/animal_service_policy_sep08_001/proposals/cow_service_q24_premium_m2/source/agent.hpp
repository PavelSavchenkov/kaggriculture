#pragma once
#include "../../../source/policy.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/source/opening.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"
namespace compositions::cow_service_q24_premium_m2 {
class Base:public cow_service::Policy {
public: Base():Policy(2,-1,-1,-1,true){}
};
class Agent:public animal_repair::Opening<premium_sales::Policy<Base,216>,24> {
public: static kag::agent::AgentInfo info(){return {"cow_service_q24_premium_m2"};}
};
}
