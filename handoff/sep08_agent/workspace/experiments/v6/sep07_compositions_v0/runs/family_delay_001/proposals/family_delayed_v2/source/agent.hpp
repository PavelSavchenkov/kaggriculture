#pragma once
#include "../../../delayed.hpp"
namespace kag::agents::family_delayed_v2{class Agent:public compositions::family_delay::Policy{public:Agent():Policy(1){} static kag::agent::AgentInfo info(){return {"family_delayed_v2"};}};}
