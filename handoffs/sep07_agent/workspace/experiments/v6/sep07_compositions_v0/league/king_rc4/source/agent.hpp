#pragma once
#include "agents/common/api/agent_api.hpp"
#include "../../public_router/source/agent.hpp"
#include <array>

namespace compositions::king_rc4 {
class Agent {
public:
    static kag::agent::AgentInfo info() { return {"king_rc4"}; }
    void reset(const kag::agent::AgentInit&);
    void act(const kag::agent::AgentObservation&, const kag::agent::DecisionBudget&, kag::Action&);
private:
    void base(const kag::agent::AgentObservation&, kag::Action&);
    void generic(const kag::agent::AgentObservation&, kag::Action&);
    void sales(const kag::agent::AgentObservation&, kag::Action&, bool throttled);
    void tail(const kag::agent::AgentObservation&, kag::Action&);
    void reserve(const kag::agent::AgentObservation&, kag::Action&);
    void rc3(const kag::agent::AgentObservation&, kag::Action&);
    compositions::public_router::Agent tail_;
    bool s1_bal_=false, low_=false, highcap_=false, yarn_second_=false, smartfarm_=false;
    int queue_day_=-1;
    std::array<bool,kag::MAX_UNITS> pending_{};
    std::array<kag::UnitAction,kag::MAX_UNITS> delayed_{};
    std::array<int,kag::N_PRODUCTS> throttle_{};
    std::array<int,kag::N_PRODUCTS> due_{};
};
}
