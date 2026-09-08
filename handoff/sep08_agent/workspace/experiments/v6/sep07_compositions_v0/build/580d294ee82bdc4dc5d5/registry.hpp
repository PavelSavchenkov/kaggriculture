#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b0_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b0_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b157_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b157_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b157_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b157_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b50_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b50_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b50_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b50_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m3/source/agent.hpp"
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
    if (name == "early_melon_b0_m0") return {std::make_unique<AgentModel<compositions::early_melon_b0_m0::Agent>>()};
    if (name == "early_melon_b0_m1") return {std::make_unique<AgentModel<compositions::early_melon_b0_m1::Agent>>()};
    if (name == "early_melon_b157_m0") return {std::make_unique<AgentModel<compositions::early_melon_b157_m0::Agent>>()};
    if (name == "early_melon_b157_m1") return {std::make_unique<AgentModel<compositions::early_melon_b157_m1::Agent>>()};
    if (name == "early_melon_b157_m2") return {std::make_unique<AgentModel<compositions::early_melon_b157_m2::Agent>>()};
    if (name == "early_melon_b157_m3") return {std::make_unique<AgentModel<compositions::early_melon_b157_m3::Agent>>()};
    if (name == "early_melon_b50_m0") return {std::make_unique<AgentModel<compositions::early_melon_b50_m0::Agent>>()};
    if (name == "early_melon_b50_m1") return {std::make_unique<AgentModel<compositions::early_melon_b50_m1::Agent>>()};
    if (name == "early_melon_b50_m2") return {std::make_unique<AgentModel<compositions::early_melon_b50_m2::Agent>>()};
    if (name == "early_melon_b50_m3") return {std::make_unique<AgentModel<compositions::early_melon_b50_m3::Agent>>()};
    if (name == "early_melon_b98_m0") return {std::make_unique<AgentModel<compositions::early_melon_b98_m0::Agent>>()};
    if (name == "early_melon_b98_m1") return {std::make_unique<AgentModel<compositions::early_melon_b98_m1::Agent>>()};
    if (name == "early_melon_b98_m2") return {std::make_unique<AgentModel<compositions::early_melon_b98_m2::Agent>>()};
    if (name == "early_melon_b98_m3") return {std::make_unique<AgentModel<compositions::early_melon_b98_m3::Agent>>()};
    std::abort();
}
