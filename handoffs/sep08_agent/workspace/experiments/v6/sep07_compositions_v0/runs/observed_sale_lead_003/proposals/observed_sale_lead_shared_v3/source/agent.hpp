#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::observed_sale_lead_shared_v3 {
class Agent:public compositions::observed_sale_lead_shared::Policy {
public:
    Agent():Policy(1){}
    static kag::agent::AgentInfo info(){return {"observed_sale_lead_shared_v3"};}
};
}
