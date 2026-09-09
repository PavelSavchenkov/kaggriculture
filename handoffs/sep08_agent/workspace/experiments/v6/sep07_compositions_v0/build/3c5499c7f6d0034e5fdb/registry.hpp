#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p14/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p146/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p157/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p26/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p38/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p50/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p98/source/agent.hpp"
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
    if (name == "cold_renewal_p0") return {std::make_unique<AgentModel<compositions::cold_renewal_p0::Agent>>()};
    if (name == "cold_renewal_p1") return {std::make_unique<AgentModel<compositions::cold_renewal_p1::Agent>>()};
    if (name == "cold_renewal_p14") return {std::make_unique<AgentModel<compositions::cold_renewal_p14::Agent>>()};
    if (name == "cold_renewal_p146") return {std::make_unique<AgentModel<compositions::cold_renewal_p146::Agent>>()};
    if (name == "cold_renewal_p157") return {std::make_unique<AgentModel<compositions::cold_renewal_p157::Agent>>()};
    if (name == "cold_renewal_p2") return {std::make_unique<AgentModel<compositions::cold_renewal_p2::Agent>>()};
    if (name == "cold_renewal_p26") return {std::make_unique<AgentModel<compositions::cold_renewal_p26::Agent>>()};
    if (name == "cold_renewal_p3") return {std::make_unique<AgentModel<compositions::cold_renewal_p3::Agent>>()};
    if (name == "cold_renewal_p38") return {std::make_unique<AgentModel<compositions::cold_renewal_p38::Agent>>()};
    if (name == "cold_renewal_p50") return {std::make_unique<AgentModel<compositions::cold_renewal_p50::Agent>>()};
    if (name == "cold_renewal_p98") return {std::make_unique<AgentModel<compositions::cold_renewal_p98::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
