#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::late_force_goose{class Agent:public compositions::late_portfolio::Policy{public:Agent():Policy(32,0,0,0,1){}static kag::agent::AgentInfo info(){return {"late_force_goose"};}};}
