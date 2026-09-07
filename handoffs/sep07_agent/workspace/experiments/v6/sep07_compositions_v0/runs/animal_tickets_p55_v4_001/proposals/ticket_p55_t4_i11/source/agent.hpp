#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t4_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{10,11,{65,1},{66,0},{69,0},{29,3},24},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t4_i11"};}};
}
