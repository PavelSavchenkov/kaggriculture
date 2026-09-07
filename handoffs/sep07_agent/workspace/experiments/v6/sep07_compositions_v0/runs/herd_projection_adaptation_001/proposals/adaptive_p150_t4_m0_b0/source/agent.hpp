#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t4_m0_b0 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{10,10,{65,1},{66,0},{69,0},{33,2},24},2,30,Service{0,536870888u,268435448u,536870904u,357917696u,0}},0,0){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t4_m0_b0"};}};}
