#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t9_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{10,11,{176,1},{180,3},{183,3},{155,2},25},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t9_i11"};}};
}
