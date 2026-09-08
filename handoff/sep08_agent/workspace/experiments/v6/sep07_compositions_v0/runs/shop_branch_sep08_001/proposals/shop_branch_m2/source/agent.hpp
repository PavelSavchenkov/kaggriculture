#pragma once
#include "../../../selector.hpp"
namespace compositions::shop_branch_m2 {
class Agent: public shop_branch::Agent<2> {
public:
    static kag::agent::AgentInfo info() { return {"shop_branch_m2"}; }
};
}
