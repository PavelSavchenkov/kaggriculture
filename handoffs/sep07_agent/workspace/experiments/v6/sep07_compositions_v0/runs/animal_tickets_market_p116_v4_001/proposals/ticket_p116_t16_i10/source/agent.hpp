#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t16_i10 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{11,10,{264,9},{266,10},{270,10},{254,6},32},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t16_i10"};}};
}
