#pragma once
#include "../../../policy/source/agent.hpp"
namespace compositions_sale_copy::opening_q32_b13_v1 {
class Agent : public opening_market_search::Agent {
public:
    Agent() : opening_market_search::Agent(32,13,2) {}
    static kag::agent::AgentInfo info() { return {"opening_q32_b13_v1"}; }
};
}
