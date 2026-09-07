#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t13_m1_b0 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{11,11,{226,2},{252,1},{258,1},{257,1},41},10,30,Service{0,503315456u,536869888u,134215680u,306774016u,0}},0,1){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t13_m1_b0"};}};}
