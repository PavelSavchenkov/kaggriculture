#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p78_t5_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{10,9,{88,1},{92,3},{95,3},{28,2},42},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p78_t5_i9"};}};
}
