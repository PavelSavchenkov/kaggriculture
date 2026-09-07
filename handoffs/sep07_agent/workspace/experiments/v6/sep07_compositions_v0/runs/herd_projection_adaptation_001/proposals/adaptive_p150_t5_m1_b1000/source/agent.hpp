#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t5_m1_b1000 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{10,10,{88,1},{92,3},{95,3},{29,3},42},3,30,Service{0,536870640u,268435440u,536870896u,715827200u,0}},1000,1){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t5_m1_b1000"};}};}
