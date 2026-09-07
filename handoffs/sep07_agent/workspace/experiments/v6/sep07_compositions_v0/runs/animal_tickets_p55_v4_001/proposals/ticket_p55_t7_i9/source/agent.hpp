#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p55_t7_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(55,AnimalEdit{10,9,{169,2},{175,3},{177,3},{156,4},46},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p55_t7_i9"};}};
}
