#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t14_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{9,11,{226,2},{253,3},{259,3},{258,3},23},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t14_i11"};}};
}
