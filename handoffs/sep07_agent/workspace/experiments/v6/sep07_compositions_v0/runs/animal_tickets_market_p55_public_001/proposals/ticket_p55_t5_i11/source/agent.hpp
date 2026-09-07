#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t5_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{10,11,{88,1},{92,3},{95,3},{28,2},42},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t5_i11"};}};
}
