#pragma once
#include "../../../routes.hpp"
#include "../../../../compiler_split_sep08_001/proposals/compiler_split_mixed/source/agent.hpp"
namespace compositions::herd_partial_mixed_m2 {
class Agent:public herd_routes_partial_sep08::Agent<compiler_split_mixed::Agent,2> {
public:
    static kag::agent::AgentInfo info() {return {"herd_partial_mixed_m2"};}
};
}
