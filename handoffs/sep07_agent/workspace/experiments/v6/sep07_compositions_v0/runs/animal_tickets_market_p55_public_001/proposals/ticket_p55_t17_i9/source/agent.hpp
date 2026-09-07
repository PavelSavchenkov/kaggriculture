#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t17_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{11,9,{265,8},{266,10},{270,10},{254,6},32},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t17_i9"};}};
}
