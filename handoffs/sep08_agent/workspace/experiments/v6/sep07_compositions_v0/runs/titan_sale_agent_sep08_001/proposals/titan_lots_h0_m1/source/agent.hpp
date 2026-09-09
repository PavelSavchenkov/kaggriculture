#pragma once
#include "../../../policy.hpp"
namespace compositions::titan_lots_h0_m1 {
class Agent:public titan_sale_agent::Agent<0,1> {
public: static kag::agent::AgentInfo info() {return {"titan_lots_h0_m1"};}
};
}
