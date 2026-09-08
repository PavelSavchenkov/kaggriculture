#pragma once
#include "../../../runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "../../../runs/investment_day_contexts_001/days.hpp"
namespace catalog_observed_sale_lead_start_216_sale::investment_context_guarded_001_best {
class Agent:public GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent> {public:Agent():GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent>(investment_context_days::select({13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127})){}
static kag::agent::AgentInfo info(){return {"investment_context_guarded_001_best"};}};
}
