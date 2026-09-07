#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::atakan_portfolio {
struct Estimate {double own=0,rival=0,min_cash=0;};
Estimate estimate(const kag::agent::AgentObservation&,int branch);
class Agent {
    int mode_=0,branch_=0;
    std::array<Estimate,3> estimates_{};
public:
    explicit Agent(int mode=0):mode_(mode),branch_(mode<3?mode:0){}
    static kag::agent::AgentInfo info(){return {"atakan_portfolio"};}
    void reset(const kag::agent::AgentInit&){branch_=mode_<3?mode_:0;estimates_={};}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int branch() const{return branch_;}
    const auto& estimates() const{return estimates_;}
};
}
