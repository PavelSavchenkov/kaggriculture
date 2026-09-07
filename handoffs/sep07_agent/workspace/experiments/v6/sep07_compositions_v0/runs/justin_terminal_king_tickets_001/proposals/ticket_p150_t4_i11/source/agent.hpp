#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_p150_t4_i11 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{10,11,{65,1},{66,0},{69,0},{33,2},24},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t4_i11"};}};
}
