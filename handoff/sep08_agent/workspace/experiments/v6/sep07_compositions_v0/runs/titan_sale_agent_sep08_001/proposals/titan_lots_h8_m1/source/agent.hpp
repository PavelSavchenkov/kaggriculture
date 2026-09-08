#pragma once
#include "../../../policy.hpp"
namespace compositions::titan_lots_h8_m1 {
class Agent:public titan_sale_agent::Agent<8,1> {
public: static kag::agent::AgentInfo info() {return {"titan_lots_h8_m1"};}
};
}
