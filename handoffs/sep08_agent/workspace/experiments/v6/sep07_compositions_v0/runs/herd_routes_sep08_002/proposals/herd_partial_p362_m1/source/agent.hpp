#pragma once
#include "../../../routes.hpp"
#include "../../../../compiler_split_sep08_001/proposals/compiler_split_p362/source/agent.hpp"
namespace compositions::herd_partial_p362_m1 {
class Agent:public herd_routes_partial_sep08::Agent<compiler_split_p362::Agent,1> {
public:
    static kag::agent::AgentInfo info() {return {"herd_partial_p362_m1"};}
};
}
