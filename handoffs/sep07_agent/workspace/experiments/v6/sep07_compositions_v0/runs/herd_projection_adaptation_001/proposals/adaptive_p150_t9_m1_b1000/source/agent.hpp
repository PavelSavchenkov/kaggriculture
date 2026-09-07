#pragma once
#include "../../../../../include/adaptive_ticket.hpp"
namespace compositions::adaptive_p150_t9_m1_b1000 {
class Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{10,10,{176,1},{180,3},{183,3},{155,2},25},7,30,Service{0,536868608u,805306240u,1073741568u,715816960u,0}},1000,1){}
static kag::agent::AgentInfo info(){return {"adaptive_p150_t9_m1_b1000"};}};}
