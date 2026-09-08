#pragma once
#include "../../../source/policy.hpp"
#include "../../../source/opening.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"
namespace compositions::premium_q24_s216 {
class Base:public animal_repair::Policy { public: Base():Policy(0){} };
class Agent:public animal_repair::Opening<premium_sales::Policy<Base,216>,24> {
public:
    static kag::agent::AgentInfo info(){return {"premium_q24_s216"};}
};
}
