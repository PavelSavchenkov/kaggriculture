#pragma once
#include "../../../../../include/animal_ticket.hpp"
#include "../../../../../include/terminal_layer.hpp"
namespace compositions::ticket_p150_t5_i11 {
using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;
class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent(150)),AnimalEdit{10,11,{88,1},{92,3},{95,3},{29,3},42},true,true){}
static kag::agent::AgentInfo info(){return {"ticket_p150_t5_i11"};}};
}
