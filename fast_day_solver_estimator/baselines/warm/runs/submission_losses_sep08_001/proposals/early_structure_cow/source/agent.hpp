#pragma once
#include "agents/external/cow_service_retained_q24_premium_m2/source/agent.hpp"
#include "../../../source/structure_repair.hpp"
namespace compositions::early_structure_cow {
class Agent:public early_structure_repair::Policy<kag::agents::cow_service_retained_q24_premium_m2::Agent> {
public: static kag::agent::AgentInfo info(){return {"early_structure_cow"};}
};
}
