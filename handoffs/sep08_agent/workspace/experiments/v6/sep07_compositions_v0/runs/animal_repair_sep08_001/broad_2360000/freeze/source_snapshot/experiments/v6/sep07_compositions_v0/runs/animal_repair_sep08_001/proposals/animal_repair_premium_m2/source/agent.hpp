#pragma once
#include "../../../source/policy.hpp"
#include "../../../source/opening.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"
namespace compositions::animal_repair_premium_m2 {
class Base:public animal_repair::Policy { public: Base():Policy(2){} };
class Agent:public premium_sales::Policy<Base,216> {
public:
    static kag::agent::AgentInfo info(){return {"animal_repair_premium_m2"};}
};
}
