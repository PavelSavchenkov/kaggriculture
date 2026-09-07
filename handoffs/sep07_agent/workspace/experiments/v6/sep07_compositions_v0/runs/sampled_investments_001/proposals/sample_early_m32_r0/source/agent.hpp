#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../general_animal_entry_bank_002/bank.hpp"
#include "../../../../../candidates/investment_context_guarded_001_best/source/agent.hpp"
namespace compositions::sample_early_m32_r0 {using Base=DeferredAnimalAgent<investment_context_guarded_001_best::Agent>;class Agent:public Base {public:Agent():Base(88,1,42,general_animal_entry_bank_002::entries(),30,9,general_animal_entry_bank_002::models(),0,0,0,10,32){} static kag::agent::AgentInfo info(){return {"sample_early_m32_r0"};}};}
