#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_days_agent_sep08_001/proposals/dated_days_combined/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_days_agent_sep08_001/proposals/dated_days_control/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_days_agent_sep08_001/proposals/dated_days_feed/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_days_agent_sep08_001/proposals/dated_days_labor/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
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
    if (name == "dated_days_combined") return {std::make_unique<AgentModel<compositions::dated_days_combined::Agent>>()};
    if (name == "dated_days_control") return {std::make_unique<AgentModel<compositions::dated_days_control::Agent>>()};
    if (name == "dated_days_feed") return {std::make_unique<AgentModel<compositions::dated_days_feed::Agent>>()};
    if (name == "dated_days_labor") return {std::make_unique<AgentModel<compositions::dated_days_labor::Agent>>()};
    if (name == "dated_expansion_p362") return {std::make_unique<AgentModel<compositions::dated_expansion_p362::Agent>>()};
    if (name == "observed_sale_lead_start_216") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_start_216::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
