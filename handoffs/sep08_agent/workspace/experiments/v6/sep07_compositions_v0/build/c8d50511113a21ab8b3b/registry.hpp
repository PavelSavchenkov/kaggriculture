#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/ahmed_v23/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_002/proposals/titan_contract_h4_m4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_002/proposals/titan_contract_h8_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_002/proposals/titan_contract_h8_m4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h0_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/titan_sale_agent_sep08_001/proposals/titan_lots_h8_m1/source/agent.hpp"
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
    if (name == "empty_sale_slots_m2") return {std::make_unique<AgentModel<compositions::empty_sale_slots_m2::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "titan_contract_h4_m4") return {std::make_unique<AgentModel<compositions::titan_contract_h4_m4::Agent>>()};
    if (name == "titan_contract_h8_m3") return {std::make_unique<AgentModel<compositions::titan_contract_h8_m3::Agent>>()};
    if (name == "titan_contract_h8_m4") return {std::make_unique<AgentModel<compositions::titan_contract_h8_m4::Agent>>()};
    if (name == "titan_lots_h0_m1") return {std::make_unique<AgentModel<compositions::titan_lots_h0_m1::Agent>>()};
    if (name == "titan_lots_h8_m1") return {std::make_unique<AgentModel<compositions::titan_lots_h8_m1::Agent>>()};
    std::abort();
}
