#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/binghua_116/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/bohann_opening_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/crop_rotation_berry_gate_001/proposals/crop_rotation_t2_berry/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_goose_wheat_context/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_portfolio_001/proposals/late_value_s32_t0_r05/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/opening_router_v4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_capacity_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_sixday/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/shop_herd_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_tickets_market_p116_v4_001/proposals/ticket_p116_t9_i9/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_fert/source/agent.hpp"
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
    if (name == "animal_adaptive_r1_c0_b0") return {std::make_unique<AgentModel<compositions::animal_adaptive_r1_c0_b0::Agent>>()};
    if (name == "binghua_116") return {std::make_unique<AgentModel<compositions::binghua_116::Agent>>()};
    if (name == "bohann_opening_v1") return {std::make_unique<AgentModel<compositions::bohann_opening_v1::Agent>>()};
    if (name == "crop_mix_t2_wheat") return {std::make_unique<AgentModel<compositions::crop_mix_t2_wheat::Agent>>()};
    if (name == "crop_rotation_t2_berry") return {std::make_unique<AgentModel<compositions::crop_rotation_t2_berry::Agent>>()};
    if (name == "crop_value_m2_t4") return {std::make_unique<AgentModel<compositions::crop_value_m2_t4::Agent>>()};
    if (name == "investment_context_guarded_001_best") return {std::make_unique<AgentModel<compositions::investment_context_guarded_001_best::Agent>>()};
    if (name == "john_131") return {std::make_unique<AgentModel<compositions::john_131::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "late_goose_wheat_context") return {std::make_unique<AgentModel<kag::agents::late_goose_wheat_context::Agent>>()};
    if (name == "late_value_s32_t0_r05") return {std::make_unique<AgentModel<kag::agents::late_value_s32_t0_r05::Agent>>()};
    if (name == "opening_q32_b13_v1") return {std::make_unique<AgentModel<compositions::opening_q32_b13_v1::Agent>>()};
    if (name == "opening_router_v4") return {std::make_unique<AgentModel<compositions::opening_router_v4::Agent>>()};
    if (name == "public_capacity_router") return {std::make_unique<AgentModel<compositions::public_capacity_router::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "public_sixday") return {std::make_unique<AgentModel<compositions::public_sixday::Agent>>()};
    if (name == "shop_herd_guarded_001_best") return {std::make_unique<AgentModel<compositions::shop_herd_guarded_001_best::Agent>>()};
    if (name == "shop_herd_s6_m3_g1") return {std::make_unique<AgentModel<compositions::shop_herd_s6_m3_g1::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "ticket_p116_t9_i9") return {std::make_unique<AgentModel<compositions::ticket_p116_t9_i9::Agent>>()};
    if (name == "wheat_one_fert") return {std::make_unique<AgentModel<compositions::wheat_one_fert::Agent>>()};
    std::abort();
}
