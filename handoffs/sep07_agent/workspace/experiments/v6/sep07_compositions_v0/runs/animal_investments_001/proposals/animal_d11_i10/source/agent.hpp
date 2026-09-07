#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../animal_entry_bank_001/bank.hpp"
#include "../../../../../candidates/shop_herd_guarded_001_best/source/agent.hpp"
namespace compositions::animal_d11_i10 {using Base=DeferredAnimalAgent<shop_herd_guarded_001_best::Agent>;class Agent:public Base{public:Agent():Base(265,7,32,animal_entry_bank_001::entries(),11,10,std::vector<AnimalInvestmentModel>{},1,0,0){}static kag::agent::AgentInfo info(){return {"animal_d11_i10"};}};}
