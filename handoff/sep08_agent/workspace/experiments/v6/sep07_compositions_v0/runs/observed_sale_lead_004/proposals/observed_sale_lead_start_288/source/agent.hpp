#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::observed_sale_lead_start_288 {
class Agent:public compositions::observed_sale_lead_after_funding::Policy {
public:
    Agent():Policy(1,288){}
    static kag::agent::AgentInfo info(){return {"observed_sale_lead_start_288"};}
};
}
