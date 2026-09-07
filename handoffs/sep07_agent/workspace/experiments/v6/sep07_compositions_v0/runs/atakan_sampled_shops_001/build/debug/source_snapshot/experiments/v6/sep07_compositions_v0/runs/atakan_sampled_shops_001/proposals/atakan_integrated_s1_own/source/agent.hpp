#pragma once
#include "../../../source/agent.hpp"
namespace compositions::atakan_integrated_s1_own { class Agent:public atakan_integrated::Agent {public:Agent():atakan_integrated::Agent(1,false){} static kag::agent::AgentInfo info(){return {"atakan_integrated_s1_own"};} }; }
