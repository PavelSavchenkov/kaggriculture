#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::late_value_s32_t100_r0{class Agent:public compositions::late_portfolio::Policy{public:Agent():Policy(32,100,0,0,-1){}static kag::agent::AgentInfo info(){return {"late_value_s32_t100_r0"};}};}
