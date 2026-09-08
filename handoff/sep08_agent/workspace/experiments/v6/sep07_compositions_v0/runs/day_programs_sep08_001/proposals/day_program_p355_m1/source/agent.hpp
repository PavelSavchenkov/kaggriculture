#pragma once
#include "../../../days.hpp"
#include "../../../../joint_day_routes_sep08_001/proposals/joint_routes_p355_m0/source/agent.hpp"
namespace compositions::day_program_p355_m1 {
class Agent:public day_programs::Agent<joint_routes_p355_m0::Agent,1> {
public:
    Agent():day_programs::Agent<joint_routes_p355_m0::Agent,1>(std::vector<day_programs::Program>{day_program_data::p355_d14_source0(),day_program_data::p355_d14_source1(),day_program_data::p355_d15_source0(),day_program_data::p355_d15_source1(),day_program_data::p355_d16_source0(),day_program_data::p355_d16_source1()}) {}
    static kag::agent::AgentInfo info() {return {"day_program_p355_m1"};}
};
}
