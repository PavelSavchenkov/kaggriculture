#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/ahmed_v23/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_001/proposals/observed_sale_lead_milk_wool/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_001/proposals/observed_sale_lead_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_001/proposals/sale_lead_prediction_control/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
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
    if (name == "ahmed_v23") return {std::make_unique<AgentModel<kag::agents::ahmed_v23::Agent>>()};
    if (name == "investment_context_guarded_001_best") return {std::make_unique<AgentModel<compositions::investment_context_guarded_001_best::Agent>>()};
    if (name == "john_131") return {std::make_unique<AgentModel<compositions::john_131::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "observed_sale_lead_milk_wool") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_milk_wool::Agent>>()};
    if (name == "observed_sale_lead_v1") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_v1::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "rival_wool_context_v3") return {std::make_unique<AgentModel<kag::agents::rival_wool_context_v3::Agent>>()};
    if (name == "sale_lead_prediction_control") return {std::make_unique<AgentModel<kag::agents::sale_lead_prediction_control::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    std::abort();
}
