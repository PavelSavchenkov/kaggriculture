#pragma once
#include "../../../include/animal_ticket.hpp"
namespace compositions::junghoon_wool_sales {
class Agent:public AnimalTicketAgent {public: Agent():AnimalTicketAgent(78,AnimalEdit{9,11,{266,2},{290,4},{300,4},{263,6},4},false,true){}
static kag::agent::AgentInfo info(){return {"junghoon_wool_sales"};}};
}
