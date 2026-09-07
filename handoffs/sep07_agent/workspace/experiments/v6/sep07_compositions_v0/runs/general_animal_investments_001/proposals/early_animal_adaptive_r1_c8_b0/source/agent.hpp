#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../general_animal_entry_bank_001/bank.hpp"
#include "../../../../../candidates/investment_context_guarded_001_best/source/agent.hpp"
namespace compositions::early_animal_adaptive_r1_c8_b0 {using Base=DeferredAnimalAgent<investment_context_guarded_001_best::Agent>;class Agent:public Base{public:Agent():Base(88,1,42,general_animal_entry_bank_001::entries(),30,9,general_animal_entry_bank_001::models(),1,8,0,10){}static kag::agent::AgentInfo info(){return {"early_animal_adaptive_r1_c8_b0"};}};}
