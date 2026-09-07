#pragma once
#include "../../../include/market_course.hpp"
#include "../../../runs/advance_sales_001/proposals/advance_sales_001_7/source/agent.hpp"
namespace kag::agents::bohann_opening_v1::detail::firstday_m85_sale7 {
class Agent:public MarketOverlayAgent<advance_sales_001_7::Agent> {
public:
    Agent():MarketOverlayAgent(85,0,23) {}
    static kag::agent::AgentInfo info() {return {"firstday_m85_sale7"};}
};
}
