#pragma once
#include "../../../source/policy.hpp"
#include "../../../../animal_repair_sep08_001/source/opening.hpp"
#include "../../../../premium_sales_sep08_001/source/policy.hpp"
namespace catalog_cow_service_retained_q24_premium_m2_compositions::cow_service_retained_q24_premium_m2 {
class Base:public cow_service_retained::Policy {
public: Base():Policy(2,-1,-1,-1,true){}
};
class Agent:public animal_repair::Opening<premium_sales::Policy<Base,216>,24> {
public: static kag::agent::AgentInfo info(){return {"cow_service_retained_q24_premium_m2"};}
};
}
