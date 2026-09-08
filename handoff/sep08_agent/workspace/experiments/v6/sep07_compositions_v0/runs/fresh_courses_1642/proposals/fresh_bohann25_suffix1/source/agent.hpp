#pragma once
#include "../../../ablation_policy.hpp"
namespace compositions::fresh_bohann25_suffix1 {
class Agent: public fresh_bohann::Policy {public:
    Agent():Policy(false,1){}
    static kag::agent::AgentInfo info(){return {"fresh_bohann25_suffix1"};}
};
}
