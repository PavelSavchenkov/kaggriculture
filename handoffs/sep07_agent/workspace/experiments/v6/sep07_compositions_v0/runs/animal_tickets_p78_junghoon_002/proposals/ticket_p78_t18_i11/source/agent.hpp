#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p78_t18_i11 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{9,11,{266,2},{290,4},{300,4},{263,6},4},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p78_t18_i11"};}};
}
