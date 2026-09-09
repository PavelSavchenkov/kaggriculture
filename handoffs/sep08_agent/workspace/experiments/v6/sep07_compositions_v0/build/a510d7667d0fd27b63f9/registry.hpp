#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_funding_sep08_001/proposals/opening_funding_q13/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_funding_sep08_001/proposals/opening_funding_q20/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_funding_sep08_001/proposals/opening_funding_q24/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_funding_sep08_001/proposals/opening_funding_q32/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
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
    if (name == "opening_funding_q13") return {std::make_unique<AgentModel<compositions::opening_funding_q13::Agent>>()};
    if (name == "opening_funding_q20") return {std::make_unique<AgentModel<compositions::opening_funding_q20::Agent>>()};
    if (name == "opening_funding_q24") return {std::make_unique<AgentModel<compositions::opening_funding_q24::Agent>>()};
    if (name == "opening_funding_q32") return {std::make_unique<AgentModel<compositions::opening_funding_q32::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
