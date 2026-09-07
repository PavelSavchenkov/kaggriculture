#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p78_t9_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{10,9,{176,1},{185,1},{188,1},{155,7},25},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p78_t9_i9"};}};
}
