#pragma once
#include "experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include "../../../source/policy.hpp"
namespace compositions::premium_sales_m0 {
class Agent:public premium_sales::Policy<compositions::empty_sale_slots_m2::Agent,720> {
public:
    static kag::agent::AgentInfo info(){return {"premium_sales_m0"};}
};
}
