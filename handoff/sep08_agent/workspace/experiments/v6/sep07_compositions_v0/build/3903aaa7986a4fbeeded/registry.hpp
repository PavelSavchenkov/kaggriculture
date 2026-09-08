#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_portfolio_001/proposals/late_value_s32_t0_r05/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v52_family_001/proposals/v52_transfer_wool0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v52_family_001/proposals/v52_transfer_wool0_h18/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v52_family_001/proposals/v52_transfer_wool2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v52_family_001/proposals/v52_transfer_wool2_h18/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v52_family_001/proposals/v52_transfer_wool4_h18/source/agent.hpp"
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
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "late_value_s32_t0_r05") return {std::make_unique<AgentModel<kag::agents::late_value_s32_t0_r05::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "v52_transfer_wool0") return {std::make_unique<AgentModel<kag::agents::v52_transfer_wool0::Agent>>()};
    if (name == "v52_transfer_wool0_h18") return {std::make_unique<AgentModel<kag::agents::v52_transfer_wool0_h18::Agent>>()};
    if (name == "v52_transfer_wool2") return {std::make_unique<AgentModel<kag::agents::v52_transfer_wool2::Agent>>()};
    if (name == "v52_transfer_wool2_h18") return {std::make_unique<AgentModel<kag::agents::v52_transfer_wool2_h18::Agent>>()};
    if (name == "v52_transfer_wool4_h18") return {std::make_unique<AgentModel<kag::agents::v52_transfer_wool4_h18::Agent>>()};
    std::abort();
}
