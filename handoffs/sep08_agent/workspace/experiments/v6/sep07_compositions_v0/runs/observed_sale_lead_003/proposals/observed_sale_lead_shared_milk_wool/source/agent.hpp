#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::observed_sale_lead_shared_milk_wool {
class Agent:public compositions::observed_sale_lead_shared::Policy {
public:
    Agent():Policy(2){}
    static kag::agent::AgentInfo info(){return {"observed_sale_lead_shared_milk_wool"};}
};
}
