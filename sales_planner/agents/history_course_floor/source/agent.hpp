#pragma once
#include "sales_planner/agents/history_course/source/agent.hpp"

namespace kag::agents::history_course_floor {
class Agent : public kag::agents::history_course::Agent {
public:
    Agent() : kag::agents::history_course::Agent(0, true, true) {}
    static kag::agent::AgentInfo info() { return {"history_course_floor"}; }
};
}
