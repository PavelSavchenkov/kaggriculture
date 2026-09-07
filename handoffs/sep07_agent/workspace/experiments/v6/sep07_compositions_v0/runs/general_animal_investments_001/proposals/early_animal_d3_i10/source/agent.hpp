#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../general_animal_entry_bank_001/bank.hpp"
#include "../../../../../candidates/investment_context_guarded_001_best/source/agent.hpp"
namespace compositions::early_animal_d3_i10 {using Base=DeferredAnimalAgent<investment_context_guarded_001_best::Agent>;class Agent:public Base{public:Agent():Base(88,1,42,general_animal_entry_bank_001::entries(),3,10,std::vector<AnimalInvestmentModel>{},1,0,0,10){}static kag::agent::AgentInfo info(){return {"early_animal_d3_i10"};}};}
