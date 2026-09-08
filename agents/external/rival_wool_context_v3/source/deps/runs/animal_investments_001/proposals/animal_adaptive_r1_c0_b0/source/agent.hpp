#pragma once
#include "../../../../../include/deferred_animal.hpp"
#include "../../../../animal_entry_bank_001/bank.hpp"
#include "../../../../../candidates/shop_herd_guarded_001_best/source/agent.hpp"
namespace catalog_rival_wool_context_v3_compositions::animal_adaptive_r1_c0_b0 {using Base=DeferredAnimalAgent<shop_herd_guarded_001_best::Agent>;class Agent:public Base{public:Agent():Base(265,7,32,animal_entry_bank_001::entries(),30,9,animal_entry_bank_001::models(),1,0,0){}static kag::agent::AgentInfo info(){return {"animal_adaptive_r1_c0_b0"};}};}
