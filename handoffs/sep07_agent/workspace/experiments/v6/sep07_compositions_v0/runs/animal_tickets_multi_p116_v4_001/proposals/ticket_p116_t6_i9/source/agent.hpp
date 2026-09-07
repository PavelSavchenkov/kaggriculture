#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t6_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{10,9,{150,3},{152,0},{156,0},{155,0},35},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t6_i9"};}};
}
