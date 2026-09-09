#pragma once
#include "../../../selector.hpp"
namespace compositions::shop_branch_m0 {
class Agent: public shop_branch::Agent<0> {
public:
    static kag::agent::AgentInfo info() { return {"shop_branch_m0"}; }
};
}
