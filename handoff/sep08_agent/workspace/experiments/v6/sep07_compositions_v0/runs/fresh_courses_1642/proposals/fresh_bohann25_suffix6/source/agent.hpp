#pragma once
#include "../../../ablation_policy.hpp"
namespace compositions::fresh_bohann25_suffix6 {
class Agent: public fresh_bohann::Policy {public:
    Agent():Policy(false,6){}
    static kag::agent::AgentInfo info(){return {"fresh_bohann25_suffix6"};}
};
}
