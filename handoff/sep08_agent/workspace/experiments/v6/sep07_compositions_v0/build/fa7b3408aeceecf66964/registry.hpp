#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h0_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h4_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h8_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h8_m2/source/agent.hpp"
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
    if (name == "empty_sale_slots_m2") return {std::make_unique<AgentModel<compositions::empty_sale_slots_m2::Agent>>()};
    if (name == "titan_lots_h0_m1") return {std::make_unique<AgentModel<compositions::titan_lots_h0_m1::Agent>>()};
    if (name == "titan_lots_h4_m1") return {std::make_unique<AgentModel<compositions::titan_lots_h4_m1::Agent>>()};
    if (name == "titan_lots_h8_m1") return {std::make_unique<AgentModel<compositions::titan_lots_h8_m1::Agent>>()};
    if (name == "titan_lots_h8_m2") return {std::make_unique<AgentModel<compositions::titan_lots_h8_m2::Agent>>()};
    std::abort();
}
