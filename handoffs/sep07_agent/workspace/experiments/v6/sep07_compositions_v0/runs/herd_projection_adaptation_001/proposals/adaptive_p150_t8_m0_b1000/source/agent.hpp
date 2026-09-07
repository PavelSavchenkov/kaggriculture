#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t8_m0_b1000 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{10,10,{169,2},{175,3},{177,3},{156,4},46},7,30,Service{0,536868608u,536870784u,1073741568u,715816960u,0}},1000,0){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t8_m0_b1000"};}};}
