#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p78_t16_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{11,9,{265,7},{266,7},{287,7},{254,2},32},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p78_t16_i9"};}};
}
