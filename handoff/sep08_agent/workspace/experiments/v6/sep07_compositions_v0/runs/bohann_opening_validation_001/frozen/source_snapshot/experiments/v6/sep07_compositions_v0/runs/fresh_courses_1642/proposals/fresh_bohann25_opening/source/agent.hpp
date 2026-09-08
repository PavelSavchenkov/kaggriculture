#pragma once
#include "../../../ablation_policy.hpp"
namespace compositions::fresh_bohann25_opening {
class Agent: public fresh_bohann::Policy {public:
    Agent():Policy(true,30){}
    static kag::agent::AgentInfo info(){return {"fresh_bohann25_opening"};}
};
}
