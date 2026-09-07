#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t12_m0_b0 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{11,11,{217,2},{219,6},{223,6},{157,7},26},9,30,Service{0,536870400u,536870400u,134216704u,155484160u,0}},0,0){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t12_m0_b0"};}};}
