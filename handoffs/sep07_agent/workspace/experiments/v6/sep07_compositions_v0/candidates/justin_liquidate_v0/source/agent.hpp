#pragma once
#include "../../../include/terminal_layer.hpp"
#include "../../../league/justin_150/source/agent.hpp"
namespace compositions::justin_liquidate_v0 {
class Agent:public TerminalAgent<justin_150::Agent,1> {
public:static kag::agent::AgentInfo info(){return {"justin_liquidate_v0"};}
};
}
