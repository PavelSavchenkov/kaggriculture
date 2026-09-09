#pragma once
#include "../../../days.hpp"
#include "../../../../day_programs_sep08_001/proposals/day_program_p362_m3/source/agent.hpp"
namespace compositions::service_bank_p362_m2 {class Agent:public day_programs::Agent<day_program_p362_m3::Agent,3> {public: Agent():day_programs::Agent<day_program_p362_m3::Agent,3>(std::vector<day_programs::Program>{service_bank_data::day16_r2()}){} static kag::agent::AgentInfo info(){return {"service_bank_p362_m2"};}};}
