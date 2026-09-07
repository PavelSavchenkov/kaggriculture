#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_terminal_p150_t9_i11 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{10,11,{176,1},{180,3},{183,3},{155,2},25},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_terminal_p150_t9_i11"};}};
}
