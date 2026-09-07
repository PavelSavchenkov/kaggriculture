#pragma once
#include "../../../source/agent.hpp"
namespace compositions::atakan_integrated_s8_margin { class Agent:public atakan_integrated::Agent {public:Agent():atakan_integrated::Agent(8,true){} static kag::agent::AgentInfo info(){return {"atakan_integrated_s8_margin"};} }; }
