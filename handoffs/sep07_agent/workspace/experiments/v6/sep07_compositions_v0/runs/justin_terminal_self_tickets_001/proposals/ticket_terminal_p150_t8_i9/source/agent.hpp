#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_terminal_p150_t8_i9 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{10,9,{169,2},{175,3},{177,3},{156,4},46},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_terminal_p150_t8_i9"};}};
}
