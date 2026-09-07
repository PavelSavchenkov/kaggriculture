#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t7_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{10,9,{151,0},{153,3},{156,3},{155,3},45},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t7_i9"};}};
}
