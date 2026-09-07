#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_p150_t16_i11 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{9,11,{265,7},{266,10},{270,10},{254,6},32},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t16_i11"};}};
}
