#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t2_i10 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{11,10,{1,5},{3,2},{7,2},{6,2},43},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t2_i10"};}};
}
