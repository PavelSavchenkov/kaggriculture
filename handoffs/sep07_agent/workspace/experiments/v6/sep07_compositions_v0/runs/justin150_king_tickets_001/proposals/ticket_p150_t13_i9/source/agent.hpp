#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p150_t13_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(150,AnimalEdit{11,9,{226,2},{252,1},{258,1},{257,1},41},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t13_i9"};}};
}
