#pragma once
#include "base/agent.hpp"
namespace compositions::atakan_demand {class Agent:public atakan_portfolio::Agent {public:Agent():atakan_portfolio::Agent(3){}static kag::agent::AgentInfo info(){return {"atakan_demand"};} };}
