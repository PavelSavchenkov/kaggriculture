#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/ahmed_v23/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m16/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m31/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/arlene_v4_sep08_001/proposals/arlene_v4_m8/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_capacity_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_terminal_router/source/agent.hpp"
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
    if (name == "arlene_v4_m0") return {std::make_unique<AgentModel<compositions::arlene_v4_m0::Agent>>()};
    if (name == "arlene_v4_m1") return {std::make_unique<AgentModel<compositions::arlene_v4_m1::Agent>>()};
    if (name == "arlene_v4_m16") return {std::make_unique<AgentModel<compositions::arlene_v4_m16::Agent>>()};
    if (name == "arlene_v4_m2") return {std::make_unique<AgentModel<compositions::arlene_v4_m2::Agent>>()};
    if (name == "arlene_v4_m31") return {std::make_unique<AgentModel<compositions::arlene_v4_m31::Agent>>()};
    if (name == "arlene_v4_m4") return {std::make_unique<AgentModel<compositions::arlene_v4_m4::Agent>>()};
    if (name == "arlene_v4_m7") return {std::make_unique<AgentModel<compositions::arlene_v4_m7::Agent>>()};
    if (name == "arlene_v4_m8") return {std::make_unique<AgentModel<compositions::arlene_v4_m8::Agent>>()};
    if (name == "john_131") return {std::make_unique<AgentModel<compositions::john_131::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "observed_sale_lead_start_216") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_start_216::Agent>>()};
    if (name == "public_capacity_router") return {std::make_unique<AgentModel<compositions::public_capacity_router::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "public_terminal_router") return {std::make_unique<AgentModel<compositions::public_terminal_router::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    std::abort();
}
