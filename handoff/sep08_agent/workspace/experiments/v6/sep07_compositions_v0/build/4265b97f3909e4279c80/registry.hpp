#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m16/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m31/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m8/source/agent.hpp"
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
    if (name == "arlene_v4_m0") return {std::make_unique<AgentModel<compositions::arlene_v4_m0::Agent>>()};
    if (name == "arlene_v4_m1") return {std::make_unique<AgentModel<compositions::arlene_v4_m1::Agent>>()};
    if (name == "arlene_v4_m16") return {std::make_unique<AgentModel<compositions::arlene_v4_m16::Agent>>()};
    if (name == "arlene_v4_m2") return {std::make_unique<AgentModel<compositions::arlene_v4_m2::Agent>>()};
    if (name == "arlene_v4_m31") return {std::make_unique<AgentModel<compositions::arlene_v4_m31::Agent>>()};
    if (name == "arlene_v4_m4") return {std::make_unique<AgentModel<compositions::arlene_v4_m4::Agent>>()};
    if (name == "arlene_v4_m7") return {std::make_unique<AgentModel<compositions::arlene_v4_m7::Agent>>()};
    if (name == "arlene_v4_m8") return {std::make_unique<AgentModel<compositions::arlene_v4_m8::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
