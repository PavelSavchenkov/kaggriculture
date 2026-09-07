#pragma once
#include "../../../runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "../../../runs/investment_day_library_001/days.hpp"
namespace compositions::investment_guarded_001_best {
class Agent:public GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent> {public:Agent():GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent>(investment_days::select({13,14,15,16,18,19,20,23,24,25,27})){}
static kag::agent::AgentInfo info(){return {"investment_guarded_001_best"};}};
}
