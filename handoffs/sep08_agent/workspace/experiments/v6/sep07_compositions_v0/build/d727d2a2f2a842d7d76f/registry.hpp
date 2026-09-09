#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/fresh_bohann25_opening/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/fresh_bohann25_opening_suffix6/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/fresh_bohann25_suffix1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/fresh_courses_1642/proposals/fresh_bohann25_suffix6/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
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
    if (name == "crop_mix_t2_wheat") return {std::make_unique<AgentModel<compositions::crop_mix_t2_wheat::Agent>>()};
    if (name == "fresh_bohann25_opening") return {std::make_unique<AgentModel<compositions::fresh_bohann25_opening::Agent>>()};
    if (name == "fresh_bohann25_opening_suffix6") return {std::make_unique<AgentModel<compositions::fresh_bohann25_opening_suffix6::Agent>>()};
    if (name == "fresh_bohann25_suffix1") return {std::make_unique<AgentModel<compositions::fresh_bohann25_suffix1::Agent>>()};
    if (name == "fresh_bohann25_suffix6") return {std::make_unique<AgentModel<compositions::fresh_bohann25_suffix6::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v5") return {std::make_unique<AgentModel<compositions::public_router_v5::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "wheat_one_fert") return {std::make_unique<AgentModel<compositions::wheat_one_fert::Agent>>()};
    std::abort();
}
