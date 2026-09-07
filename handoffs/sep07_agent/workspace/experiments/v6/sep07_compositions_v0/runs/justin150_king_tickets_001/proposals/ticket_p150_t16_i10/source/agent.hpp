#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p150_t16_i10 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(150,AnimalEdit{9,10,{265,7},{266,10},{270,10},{254,6},32},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t16_i10"};}};
}
