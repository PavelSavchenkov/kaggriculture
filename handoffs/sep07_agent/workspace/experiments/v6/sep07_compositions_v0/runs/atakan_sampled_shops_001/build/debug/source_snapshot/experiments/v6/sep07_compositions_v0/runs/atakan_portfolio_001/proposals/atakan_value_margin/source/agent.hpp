#pragma once
#include "../../../source/agent.hpp"
namespace compositions::atakan_value_margin {class Agent:public atakan_portfolio::Agent {public:Agent():atakan_portfolio::Agent(6){}static kag::agent::AgentInfo info(){return {"atakan_value_margin"};} };}
