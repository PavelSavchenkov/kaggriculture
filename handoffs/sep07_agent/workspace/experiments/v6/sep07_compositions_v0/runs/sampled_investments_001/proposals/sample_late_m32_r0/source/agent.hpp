#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../animal_entry_bank_001/bank.hpp"
#include "../../../../../candidates/shop_herd_guarded_001_best/source/agent.hpp"
#include "../../../../investment_day_contexts_001/days.hpp"
namespace compositions::sample_late_m32_r0 {using Base=DeferredAnimalAgent<shop_herd_guarded_001_best::Agent>;class Agent:public GuardedDayAgent<Base> {public:Agent():GuardedDayAgent<Base>(investment_context_days::select({13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127}),Base(265,7,32,animal_entry_bank_001::entries(),30,9,animal_entry_bank_001::models(),0,0,0,9,32)){} static kag::agent::AgentInfo info(){return {"sample_late_m32_r0"};}};}
