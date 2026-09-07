#pragma once
#include "../../../../../include/animal_ticket.hpp"
namespace compositions::ticket_p116_t15_i9 {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(116,AnimalEdit{11,9,{252,2},{254,11},{260,11},{259,11},14},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p116_t15_i9"};}};
}
