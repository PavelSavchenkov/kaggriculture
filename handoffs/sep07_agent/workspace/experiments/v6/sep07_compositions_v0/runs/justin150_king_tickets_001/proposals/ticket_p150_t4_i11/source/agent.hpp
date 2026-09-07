#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p150_t4_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(150,AnimalEdit{10,11,{65,1},{66,0},{69,0},{33,2},24},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t4_i11"};}};
}
