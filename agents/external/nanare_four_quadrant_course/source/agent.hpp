#pragma once
#include "agents/common/api/agent_api.hpp"
namespace kag::agents::nanare_four_quadrant_course {
class Agent {
public:
 static kag::agent::AgentInfo info(){return {"nanare_four_quadrant_course"};}
 void reset(const kag::agent::AgentInit&){}
 void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
