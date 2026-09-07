#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t8_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{10,11,{195,4},{196,2},{199,2},{161,3},36},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t8_i11"};}};
}
