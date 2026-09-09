#pragma once
#include "../../../parent_source/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
namespace compositions::crop_context_m1 {
class Agent:public compositions_crop_context::empty_sale_slots_m2::Agent {
public:
    Agent(){set_crop_mode(1);}
    static kag::agent::AgentInfo info(){return {"crop_context_m1"};}
};
}
