#pragma once
#include "../../../include/market_course.hpp"
#include "../../../runs/advance_sales_001/proposals/advance_sales_001_7/source/agent.hpp"
namespace catalog_rival_wool_purchase_repair_v1_compositions::firstday_m85_sale7 {
class Agent:public MarketOverlayAgent<advance_sales_001_7::Agent> {
public:
    Agent():MarketOverlayAgent(85,0,23) {}
    static kag::agent::AgentInfo info() {return {"firstday_m85_sale7"};}
};
}
