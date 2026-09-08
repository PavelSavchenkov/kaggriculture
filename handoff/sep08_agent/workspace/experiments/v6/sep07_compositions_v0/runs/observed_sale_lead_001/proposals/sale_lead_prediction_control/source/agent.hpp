#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::sale_lead_prediction_control {
class Agent:public compositions::observed_sale_lead::Policy {
public:
    Agent():Policy(0){}
    static kag::agent::AgentInfo info(){return {"sale_lead_prediction_control"};}
};
}
