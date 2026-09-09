#pragma once
#include "experiments/v6/sep07_compositions_v0/league/ahmed_v23/source/agent.hpp"
#include "../../../source/policy.hpp"
namespace compositions::ahmed_v24 {
class Agent:public premium_sales::Policy<kag::agents::ahmed_v23::Agent,144> {
public:
    static kag::agent::AgentInfo info(){return {"ahmed_v24"};}
};
}
