#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_p150_t13_i9 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{11,9,{226,2},{252,1},{258,1},{257,1},41},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t13_i9"};}};
}
