#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>
namespace compositions::atakan_integrated {
struct Estimate {double own=0,rival=0,min_cash=0;};
std::array<Estimate,3> estimate_all(const kag::agent::AgentObservation&,int count);
class Agent {
    int count_=0,branch_=0;bool margin_=false;
    std::array<Estimate,3> estimates_{};
public:
    Agent(int count=0,bool margin=false):count_(count),margin_(margin){}
    static kag::agent::AgentInfo info(){return {"atakan_integrated"};}
    void reset(const kag::agent::AgentInit&){branch_=0;estimates_={};}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int branch()const{return branch_;}
    const auto&estimates()const{return estimates_;}
};
}
