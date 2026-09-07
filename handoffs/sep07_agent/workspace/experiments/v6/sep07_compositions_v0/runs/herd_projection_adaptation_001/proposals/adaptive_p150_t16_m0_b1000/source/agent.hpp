#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t16_m0_b1000 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{9,9,{265,7},{266,10},{270,10},{254,6},32},11,30,Service{0,520091648u,536868864u,100659200u,630325248u,0}},1000,0){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t16_m0_b1000"};}};}
