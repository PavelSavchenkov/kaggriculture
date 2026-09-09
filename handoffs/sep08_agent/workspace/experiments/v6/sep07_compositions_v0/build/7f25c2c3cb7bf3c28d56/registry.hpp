#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_mixed_b1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_mixed_b2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_mixed_b4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p355_b1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p355_b2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p355_b4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p362_b1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p362_b2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p362_b4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p4_b1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p4_b2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p4_b4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p55_b1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p55_b2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_feed_bundle_sep08_001/proposals/compiler_feed_p55_b4/source/agent.hpp"
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
    if (name == "compiler_feed_mixed_b1") return {std::make_unique<AgentModel<compositions::compiler_feed_mixed_b1::Agent>>()};
    if (name == "compiler_feed_mixed_b2") return {std::make_unique<AgentModel<compositions::compiler_feed_mixed_b2::Agent>>()};
    if (name == "compiler_feed_mixed_b4") return {std::make_unique<AgentModel<compositions::compiler_feed_mixed_b4::Agent>>()};
    if (name == "compiler_feed_p355_b1") return {std::make_unique<AgentModel<compositions::compiler_feed_p355_b1::Agent>>()};
    if (name == "compiler_feed_p355_b2") return {std::make_unique<AgentModel<compositions::compiler_feed_p355_b2::Agent>>()};
    if (name == "compiler_feed_p355_b4") return {std::make_unique<AgentModel<compositions::compiler_feed_p355_b4::Agent>>()};
    if (name == "compiler_feed_p362_b1") return {std::make_unique<AgentModel<compositions::compiler_feed_p362_b1::Agent>>()};
    if (name == "compiler_feed_p362_b2") return {std::make_unique<AgentModel<compositions::compiler_feed_p362_b2::Agent>>()};
    if (name == "compiler_feed_p362_b4") return {std::make_unique<AgentModel<compositions::compiler_feed_p362_b4::Agent>>()};
    if (name == "compiler_feed_p4_b1") return {std::make_unique<AgentModel<compositions::compiler_feed_p4_b1::Agent>>()};
    if (name == "compiler_feed_p4_b2") return {std::make_unique<AgentModel<compositions::compiler_feed_p4_b2::Agent>>()};
    if (name == "compiler_feed_p4_b4") return {std::make_unique<AgentModel<compositions::compiler_feed_p4_b4::Agent>>()};
    if (name == "compiler_feed_p55_b1") return {std::make_unique<AgentModel<compositions::compiler_feed_p55_b1::Agent>>()};
    if (name == "compiler_feed_p55_b2") return {std::make_unique<AgentModel<compositions::compiler_feed_p55_b2::Agent>>()};
    if (name == "compiler_feed_p55_b4") return {std::make_unique<AgentModel<compositions::compiler_feed_p55_b4::Agent>>()};
    if (name == "observed_sale_lead_start_216") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_start_216::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
