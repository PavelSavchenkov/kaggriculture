#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t8_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{10,9,{173,0},{175,3},{177,3},{156,4},46},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t8_i9"};}};
}
