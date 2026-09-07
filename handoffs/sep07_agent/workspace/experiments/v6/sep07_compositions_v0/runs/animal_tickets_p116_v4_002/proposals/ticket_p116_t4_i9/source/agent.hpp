#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t4_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{10,9,{65,1},{66,0},{69,0},{29,3},24},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t4_i9"};}};
}
