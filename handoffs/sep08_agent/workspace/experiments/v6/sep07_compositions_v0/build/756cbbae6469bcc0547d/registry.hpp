#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "agents/external/boatlee_h7_fast_sanitize/source/agent.hpp"
#include "agents/external/c68_h18_fast_sanitize/source/agent.hpp"
#include "agents/external/public_indar_e279_unchanged/source/agent.hpp"
#include "agents/external/public_kaito_h10_front_run_fast/source/agent.hpp"
#include "agents/external/public_skomuro_2000_cpp/source/agent.hpp"
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
    if (name == "boatlee") return {std::make_unique<AgentModel<four_shop_foundry::throughput::boatlee_h7_fast_sanitize::Agent>>()};
    if (name == "c68") return {std::make_unique<AgentModel<four_shop_foundry::throughput::c68_h18_fast_sanitize::Agent>>()};
    if (name == "indar") return {std::make_unique<AgentModel<league::public_unchanged::indar::Policy>>()};
    if (name == "kaito") return {std::make_unique<AgentModel<league::public_optimized::kaito_h10_front_run_fast::Policy>>()};
    if (name == "skomuro") return {std::make_unique<AgentModel<four_shop_search::public_skomuro_2000_cpp::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    std::abort();
}
