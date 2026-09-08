#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/day_service_bank_sep08_001/proposals/service_bank_p362_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_branch_sep08_001/proposals/shop_branch_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_branch_sep08_001/proposals/shop_branch_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_branch_sep08_001/proposals/shop_branch_m2/source/agent.hpp"
struct AnyAgent {
    virtual ~AnyAgent() = default;
    virtual void reset(const kag::agent::AgentInit&) = 0;
    virtual void act(const kag::agent::AgentObservation&, const kag::agent::DecisionBudget&, kag::Action&) = 0;
};
template<class T> struct AgentModel : AnyAgent {
    T value;
    void reset(const kag::agent::AgentInit& i) override { value.reset(i); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) override { value.act(o,b,a); }
};
struct AgentBox {
    std::unique_ptr<AnyAgent> value;
    void reset(const kag::agent::AgentInit& i) { value->reset(i); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { value->act(o,b,a); }
};
inline AgentBox make_agent(const std::string& name) {
    if (name == "pass") return {std::make_unique<AgentModel<compositions::Pass>>()};
    if (name == "service_bank_p362_m2") return {std::make_unique<AgentModel<compositions::service_bank_p362_m2::Agent>>()};
    if (name == "shop_branch_m0") return {std::make_unique<AgentModel<compositions::shop_branch_m0::Agent>>()};
    if (name == "shop_branch_m1") return {std::make_unique<AgentModel<compositions::shop_branch_m1::Agent>>()};
    if (name == "shop_branch_m2") return {std::make_unique<AgentModel<compositions::shop_branch_m2::Agent>>()};
    std::abort();
}
