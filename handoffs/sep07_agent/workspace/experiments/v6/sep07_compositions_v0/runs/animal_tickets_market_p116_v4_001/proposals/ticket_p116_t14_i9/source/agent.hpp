#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t14_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{11,9,{251,2},{253,3},{259,3},{258,3},23},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t14_i9"};}};
}
