#pragma once
#include "../../../include/market_course.hpp"
#include "../../../runs/advance_sales_001/proposals/advance_sales_001_7/source/agent.hpp"
namespace catalog_late_value_s32_t0_r05_compositions::firstday_m85_sale7 {
class Agent:public MarketOverlayAgent<advance_sales_001_7::Agent> {
public:
    Agent():MarketOverlayAgent(85,0,23) {}
    static kag::agent::AgentInfo info() {return {"firstday_m85_sale7"};}
};
}
