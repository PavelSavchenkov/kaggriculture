#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t13_i10 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{9,10,{217,1},{252,1},{258,1},{257,1},41},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t13_i10"};}};
}
