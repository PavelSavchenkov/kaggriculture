#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::rival_wool_context_v3{class Agent:public compositions::rival_wool_context_v3::Policy{public:Agent():Policy(2){} static kag::agent::AgentInfo info(){return {"rival_wool_context_v3"};}};}
