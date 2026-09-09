#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/bohann_opening_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_cow_initial/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_crop_control/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_goose_initial/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_goose_optimized/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_goose_optimized_off/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_goose_optimized_on/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_animal_schedule_001/proposals/late_sheep_initial/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_sixday/source/agent.hpp"
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
    if (name == "bohann_opening_v1") return {std::make_unique<AgentModel<compositions::bohann_opening_v1::Agent>>()};
    if (name == "investment_context_guarded_001_best") return {std::make_unique<AgentModel<compositions::investment_context_guarded_001_best::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "late_cow_initial") return {std::make_unique<AgentModel<kag::agents::late_cow_initial::Agent>>()};
    if (name == "late_crop_control") return {std::make_unique<AgentModel<kag::agents::late_crop_control::Agent>>()};
    if (name == "late_goose_initial") return {std::make_unique<AgentModel<kag::agents::late_goose_initial::Agent>>()};
    if (name == "late_goose_optimized") return {std::make_unique<AgentModel<kag::agents::late_goose_optimized::Agent>>()};
    if (name == "late_goose_optimized_off") return {std::make_unique<AgentModel<kag::agents::late_goose_optimized_off::Agent>>()};
    if (name == "late_goose_optimized_on") return {std::make_unique<AgentModel<kag::agents::late_goose_optimized_on::Agent>>()};
    if (name == "late_sheep_initial") return {std::make_unique<AgentModel<kag::agents::late_sheep_initial::Agent>>()};
    if (name == "opening_q32_b13_v1") return {std::make_unique<AgentModel<compositions::opening_q32_b13_v1::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "public_sixday") return {std::make_unique<AgentModel<compositions::public_sixday::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    std::abort();
}
