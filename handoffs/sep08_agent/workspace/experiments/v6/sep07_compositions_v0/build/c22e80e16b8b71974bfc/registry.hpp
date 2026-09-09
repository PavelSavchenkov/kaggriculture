#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/cold_renewal_sep08_001/proposals/cold_renewal_p98/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/early_melon_sep08_001/proposals/early_melon_b98_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_day_routes_sep08_001/proposals/joint_routes_p362_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/day_service_bank_sep08_001/proposals/service_bank_p362_m2/source/agent.hpp"
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
    if (name == "cold_renewal_p98") return {std::make_unique<AgentModel<compositions::cold_renewal_p98::Agent>>()};
    if (name == "early_melon_b98_m1") return {std::make_unique<AgentModel<compositions::early_melon_b98_m1::Agent>>()};
    if (name == "early_melon_b98_m2") return {std::make_unique<AgentModel<compositions::early_melon_b98_m2::Agent>>()};
    if (name == "empty_sale_slots_m2") return {std::make_unique<AgentModel<compositions::empty_sale_slots_m2::Agent>>()};
    if (name == "joint_routes_p362_m0") return {std::make_unique<AgentModel<compositions::joint_routes_p362_m0::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "service_bank_p362_m2") return {std::make_unique<AgentModel<compositions::service_bank_p362_m2::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    std::abort();
}
