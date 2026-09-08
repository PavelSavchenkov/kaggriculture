#pragma once
#include "../../../routes.hpp"
#include "../../../../compiler_split_sep08_001/proposals/compiler_split_goose/source/agent.hpp"
namespace compositions::herd_routes_goose_m1 {
class Agent:public herd_routes_sep08::Agent<compiler_split_goose::Agent,1> {
public:
    static kag::agent::AgentInfo info() {return {"herd_routes_goose_m1"};}
};
}
