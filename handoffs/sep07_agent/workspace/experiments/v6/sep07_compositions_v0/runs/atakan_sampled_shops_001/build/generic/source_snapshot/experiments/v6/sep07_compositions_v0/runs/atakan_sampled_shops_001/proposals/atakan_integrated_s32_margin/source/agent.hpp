#pragma once
#include "../../../source/agent.hpp"
namespace compositions::atakan_integrated_s32_margin { class Agent:public atakan_integrated::Agent {public:Agent():atakan_integrated::Agent(32,true){} static kag::agent::AgentInfo info(){return {"atakan_integrated_s32_margin"};} }; }
