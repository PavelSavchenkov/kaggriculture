#pragma once
#include "../../../policy/source/agent.hpp"
namespace compositions::opening_q81_b13_v1 {
class Agent : public opening_market_search::Agent {
public:
    Agent() : opening_market_search::Agent(81,13,2) {}
    static kag::agent::AgentInfo info() { return {"opening_q81_b13_v1"}; }
};
}
