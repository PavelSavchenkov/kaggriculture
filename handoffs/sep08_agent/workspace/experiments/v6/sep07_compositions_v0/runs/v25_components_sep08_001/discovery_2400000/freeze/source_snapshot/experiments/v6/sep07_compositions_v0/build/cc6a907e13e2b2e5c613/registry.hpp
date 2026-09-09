#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/ahmed_v23/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/proposals/ahmed_v24/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/ahmed_v25_sep08_001/proposals/ahmed_v25/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/junghoon_wool_sales/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m6/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/v25_components_sep08_001/proposals/v25_components_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/yusuke_port_sep08_001/proposals/yusuke_sep08_m2/source/agent.hpp"
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
    if (name == "ahmed_v24") return {std::make_unique<AgentModel<compositions::ahmed_v24::Agent>>()};
    if (name == "ahmed_v25") return {std::make_unique<AgentModel<compositions::ahmed_v25::Agent>>()};
    if (name == "animal_repair_q24_premium_m2") return {std::make_unique<AgentModel<compositions::animal_repair_q24_premium_m2::Agent>>()};
    if (name == "junghoon_wool_sales") return {std::make_unique<AgentModel<compositions::junghoon_wool_sales::Agent>>()};
    if (name == "king_rc4") return {std::make_unique<AgentModel<compositions::king_rc4::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "public_router_v52") return {std::make_unique<AgentModel<kag::agents::public_router_v52::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "v25_components_m0") return {std::make_unique<AgentModel<compositions::v25_components_m0::Agent>>()};
    if (name == "v25_components_m1") return {std::make_unique<AgentModel<compositions::v25_components_m1::Agent>>()};
    if (name == "v25_components_m2") return {std::make_unique<AgentModel<compositions::v25_components_m2::Agent>>()};
    if (name == "v25_components_m3") return {std::make_unique<AgentModel<compositions::v25_components_m3::Agent>>()};
    if (name == "v25_components_m4") return {std::make_unique<AgentModel<compositions::v25_components_m4::Agent>>()};
    if (name == "v25_components_m5") return {std::make_unique<AgentModel<compositions::v25_components_m5::Agent>>()};
    if (name == "v25_components_m6") return {std::make_unique<AgentModel<compositions::v25_components_m6::Agent>>()};
    if (name == "v25_components_m7") return {std::make_unique<AgentModel<compositions::v25_components_m7::Agent>>()};
    if (name == "yusuke_sep08_m2") return {std::make_unique<AgentModel<compositions::yusuke_sep08_m2::Agent>>()};
    std::abort();
}
