#pragma once
#include "../../../source/agent.hpp"
namespace compositions::arlene_v4_m31 {
class Agent:public arlene_v4_sep08::AgentCore {
public:
    Agent():AgentCore(31) {}
    static kag::agent::AgentInfo info() {return {"arlene_v4_m31"};}
};
}
