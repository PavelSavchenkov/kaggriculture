#pragma once
#include "../../../days.hpp"
#include "../../../../joint_day_routes_sep08_001/proposals/joint_routes_p355_m0/source/agent.hpp"
namespace compositions::day_program_p355_m0 {
class Agent:public day_programs::Agent<joint_routes_p355_m0::Agent,0> {
public:
    Agent():day_programs::Agent<joint_routes_p355_m0::Agent,0>(std::vector<day_programs::Program>{}) {}
    static kag::agent::AgentInfo info() {return {"day_program_p355_m0"};}
};
}
