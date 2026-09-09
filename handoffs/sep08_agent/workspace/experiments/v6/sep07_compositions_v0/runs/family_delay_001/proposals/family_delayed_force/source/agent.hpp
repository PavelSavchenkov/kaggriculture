#pragma once
#include "../../../delayed.hpp"
namespace kag::agents::family_delayed_force{class Agent:public compositions::family_delay::Policy{public:Agent():Policy(0){} static kag::agent::AgentInfo info(){return {"family_delayed_force"};}};}
