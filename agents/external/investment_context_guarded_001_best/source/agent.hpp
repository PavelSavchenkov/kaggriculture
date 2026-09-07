#pragma once
#include "deps/experiments/v6/sep07_compositions_v0/runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "deps/experiments/v6/sep07_compositions_v0/runs/investment_day_contexts_001/days.hpp"
namespace compositions::investment_context_guarded_001_best {
class Agent:public GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent> {public:Agent():GuardedDayAgent<animal_adaptive_r1_c0_b0::Agent>(investment_context_days::select({13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127})){}
static kag::agent::AgentInfo info(){return {"investment_context_guarded_001_best"};}};
}
