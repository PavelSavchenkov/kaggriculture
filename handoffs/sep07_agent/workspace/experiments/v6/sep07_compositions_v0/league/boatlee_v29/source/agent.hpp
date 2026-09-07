#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::boatlee_v29 {
class Agent {
    std::array<int,kag::MAX_UNITS> weed_start_{};
    std::array<kag::UnitAction,kag::MAX_UNITS> weed_intended_{};
    std::array<int,3> last_inventory_{},last_sold_{},added_{};
    std::array<double,3> pressure_{};
    std::array<uint8_t,kag::N_SHOPS> last_shops_{};
    int last_step_=-1,last_shop_count_=0,mirror_=-1;
    void weed(const kag::agent::AgentObservation&,kag::Action&);
    void market(const kag::agent::AgentObservation&,kag::Action&);
public:
    static kag::agent::AgentInfo info(){return {"boatlee_v29"};}
    void reset(const kag::agent::AgentInit&);
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
