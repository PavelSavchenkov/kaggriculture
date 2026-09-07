#pragma once
#include "../../../source/agent.hpp"
namespace compositions::atakan_quotes {class Agent:public atakan_portfolio::Agent {public:Agent():atakan_portfolio::Agent(4){}static kag::agent::AgentInfo info(){return {"atakan_quotes"};} };}
