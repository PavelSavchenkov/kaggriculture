#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t3_i10 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{11,10,{2,2},{4,4},{9,4},{8,4},33},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t3_i10"};}};
}
