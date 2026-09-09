#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_goose/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_mixed/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_p355/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_p362/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_p4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_split_sep08_001/proposals/compiler_split_p55/source/agent.hpp"
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
    if (name == "compiler_split_goose") return {std::make_unique<AgentModel<compositions::compiler_split_goose::Agent>>()};
    if (name == "compiler_split_mixed") return {std::make_unique<AgentModel<compositions::compiler_split_mixed::Agent>>()};
    if (name == "compiler_split_p355") return {std::make_unique<AgentModel<compositions::compiler_split_p355::Agent>>()};
    if (name == "compiler_split_p362") return {std::make_unique<AgentModel<compositions::compiler_split_p362::Agent>>()};
    if (name == "compiler_split_p4") return {std::make_unique<AgentModel<compositions::compiler_split_p4::Agent>>()};
    if (name == "compiler_split_p55") return {std::make_unique<AgentModel<compositions::compiler_split_p55::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
