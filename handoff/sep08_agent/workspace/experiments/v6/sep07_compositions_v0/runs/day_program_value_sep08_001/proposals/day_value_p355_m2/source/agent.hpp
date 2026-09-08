#pragma once
#include "../../../policy.hpp"
#include "../../../../joint_day_routes_sep08_001/proposals/joint_routes_p355_m0/source/agent.hpp"
namespace compositions::day_value_p355_m2 {
class Agent:public day_program_value::Agent<joint_routes_p355_m0::Agent,3> {
public:Agent():day_program_value::Agent<joint_routes_p355_m0::Agent,3>(std::vector<day_programs::Program>{day_program_data::p355_d14_source0(),day_program_data::p355_d14_source1(),day_program_data::p355_d15_source0(),day_program_data::p355_d15_source1(),day_program_data::p355_d16_source0(),day_program_data::p355_d16_source1()}) {}
static kag::agent::AgentInfo info(){return {"day_value_p355_m2"};}
};
}
