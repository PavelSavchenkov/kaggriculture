#pragma once
#include "../../../source/agent.hpp"
namespace compositions::sheep_fixed_expansion{class Agent:public sheep_portfolio::Agent{public:Agent():sheep_portfolio::Agent(1){}static kag::agent::AgentInfo info(){return {"sheep_fixed_expansion"};}};}
