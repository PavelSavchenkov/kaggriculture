#pragma once
#include "../../../source/policy.hpp"
#include "../../../source/opening.hpp"
#include "../../../../premium_sales_sep08_001/source/policy.hpp"
namespace catalog_animal_repair_q24_premium_m2_compositions::animal_repair_q24_premium_m2 {
class Base:public animal_repair::Policy { public: Base():Policy(2){} };
class Agent:public animal_repair::Opening<premium_sales::Policy<Base,216>,24> {
public:
    static kag::agent::AgentInfo info(){return {"animal_repair_q24_premium_m2"};}
};
}
