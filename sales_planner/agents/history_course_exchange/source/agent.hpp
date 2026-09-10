#pragma once
#include "sales_planner/agents/history_course/source/agent.hpp"

namespace kag::agents::history_course_exchange {
class Agent : public kag::agents::history_course::Agent {
public:
    Agent() : kag::agents::history_course::Agent(0, true, true, true, true, true, true) {}
    static kag::agent::AgentInfo info() { return {"history_course_exchange"}; }
};
}
