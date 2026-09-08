#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>
namespace compositions::sheep_portfolio {
struct Estimate {double own=0,rival=0,min_cash=0;};
std::array<Estimate,2> estimate_all(const kag::agent::AgentObservation&,int count);
class Agent {
    int mode_=0,branch_=0;std::array<Estimate,2> estimates_{};
public:
    explicit Agent(int mode=0):mode_(mode),branch_(mode<2?mode:0){}
    static kag::agent::AgentInfo info(){return {"sheep_portfolio"};}
    void reset(const kag::agent::AgentInit&){branch_=mode_<2?mode_:0;estimates_={};}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int branch()const{return branch_;}const auto&estimates()const{return estimates_;}
};}
