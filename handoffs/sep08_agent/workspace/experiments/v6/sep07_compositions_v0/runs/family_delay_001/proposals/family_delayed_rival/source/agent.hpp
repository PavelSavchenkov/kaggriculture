#pragma once
#include "../../../delayed.hpp"
namespace kag::agents::family_delayed_rival{class Agent:public compositions::family_delay::Policy{public:Agent():Policy(2){} static kag::agent::AgentInfo info(){return {"family_delayed_rival"};}};}
