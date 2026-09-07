#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p78_t8_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{10,11,{169,1},{178,5},{180,5},{155,3},46},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p78_t8_i11"};}};
}
