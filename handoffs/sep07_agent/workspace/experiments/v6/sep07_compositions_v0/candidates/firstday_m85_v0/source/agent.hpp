#pragma once
#include "../../../include/market_course.hpp"
namespace compositions::firstday_m85_v0 {
class Agent:public MarketCourseAgent {
public:
    Agent():MarketCourseAgent(55,85,0,23) {}
    static kag::agent::AgentInfo info() {return {"firstday_m85_v0"};}
};
}
