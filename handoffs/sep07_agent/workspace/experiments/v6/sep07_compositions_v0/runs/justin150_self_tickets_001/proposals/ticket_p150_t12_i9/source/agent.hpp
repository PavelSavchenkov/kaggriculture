#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p150_t12_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(150,AnimalEdit{11,9,{217,2},{219,6},{223,6},{157,7},26},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t12_i9"};}};
}
