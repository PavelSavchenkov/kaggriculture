#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/rival_wool_repair_001/proposals/rival_wool_purchase_repair_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/wool_contract_repair_002/proposals/wool_contract_repair_v2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/wool_family_context_v2_001/proposals/wool_family_context_v2/source/agent.hpp"
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
    if (name == "john_131") return {std::make_unique<AgentModel<compositions::john_131::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "rival_wool_context_v3") return {std::make_unique<AgentModel<kag::agents::rival_wool_context_v3::Agent>>()};
    if (name == "rival_wool_purchase_repair_v1") return {std::make_unique<AgentModel<kag::agents::rival_wool_purchase_repair_v1::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "wool_contract_repair_v2") return {std::make_unique<AgentModel<kag::agents::wool_contract_repair_v2::Agent>>()};
    if (name == "wool_family_context_v2") return {std::make_unique<AgentModel<kag::agents::wool_family_context_v2::Agent>>()};
    std::abort();
}
