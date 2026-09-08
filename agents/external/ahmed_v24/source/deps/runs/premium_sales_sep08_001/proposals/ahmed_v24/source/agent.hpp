#pragma once
#include "../../../../../league/ahmed_v23/source/agent.hpp"
#include "../../../source/policy.hpp"
namespace catalog_ahmed_v24_compositions::ahmed_v24 {
class Agent:public premium_sales::Policy<kag::catalog_ahmed_v24_agents::ahmed_v23::Agent,144> {
public:
    static kag::agent::AgentInfo info(){return {"ahmed_v24"};}
};
}
