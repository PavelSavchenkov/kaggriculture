#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_goose_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_goose_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_goose_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_goose_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_goose_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_mixed_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_mixed_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_mixed_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_mixed_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_mixed_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p355_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p355_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p355_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p355_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p355_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p362_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p362_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p362_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p362_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p362_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p4_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p4_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p4_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p4_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p4_m7/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p55_m0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p55_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p55_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p55_m3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/joint_resource_routes_sep08_001/proposals/joint_resources_p55_m7/source/agent.hpp"
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
    if (name == "joint_resources_goose_m0") return {std::make_unique<AgentModel<compositions::joint_resources_goose_m0::Agent>>()};
    if (name == "joint_resources_goose_m1") return {std::make_unique<AgentModel<compositions::joint_resources_goose_m1::Agent>>()};
    if (name == "joint_resources_goose_m2") return {std::make_unique<AgentModel<compositions::joint_resources_goose_m2::Agent>>()};
    if (name == "joint_resources_goose_m3") return {std::make_unique<AgentModel<compositions::joint_resources_goose_m3::Agent>>()};
    if (name == "joint_resources_goose_m7") return {std::make_unique<AgentModel<compositions::joint_resources_goose_m7::Agent>>()};
    if (name == "joint_resources_mixed_m0") return {std::make_unique<AgentModel<compositions::joint_resources_mixed_m0::Agent>>()};
    if (name == "joint_resources_mixed_m1") return {std::make_unique<AgentModel<compositions::joint_resources_mixed_m1::Agent>>()};
    if (name == "joint_resources_mixed_m2") return {std::make_unique<AgentModel<compositions::joint_resources_mixed_m2::Agent>>()};
    if (name == "joint_resources_mixed_m3") return {std::make_unique<AgentModel<compositions::joint_resources_mixed_m3::Agent>>()};
    if (name == "joint_resources_mixed_m7") return {std::make_unique<AgentModel<compositions::joint_resources_mixed_m7::Agent>>()};
    if (name == "joint_resources_p355_m0") return {std::make_unique<AgentModel<compositions::joint_resources_p355_m0::Agent>>()};
    if (name == "joint_resources_p355_m1") return {std::make_unique<AgentModel<compositions::joint_resources_p355_m1::Agent>>()};
    if (name == "joint_resources_p355_m2") return {std::make_unique<AgentModel<compositions::joint_resources_p355_m2::Agent>>()};
    if (name == "joint_resources_p355_m3") return {std::make_unique<AgentModel<compositions::joint_resources_p355_m3::Agent>>()};
    if (name == "joint_resources_p355_m7") return {std::make_unique<AgentModel<compositions::joint_resources_p355_m7::Agent>>()};
    if (name == "joint_resources_p362_m0") return {std::make_unique<AgentModel<compositions::joint_resources_p362_m0::Agent>>()};
    if (name == "joint_resources_p362_m1") return {std::make_unique<AgentModel<compositions::joint_resources_p362_m1::Agent>>()};
    if (name == "joint_resources_p362_m2") return {std::make_unique<AgentModel<compositions::joint_resources_p362_m2::Agent>>()};
    if (name == "joint_resources_p362_m3") return {std::make_unique<AgentModel<compositions::joint_resources_p362_m3::Agent>>()};
    if (name == "joint_resources_p362_m7") return {std::make_unique<AgentModel<compositions::joint_resources_p362_m7::Agent>>()};
    if (name == "joint_resources_p4_m0") return {std::make_unique<AgentModel<compositions::joint_resources_p4_m0::Agent>>()};
    if (name == "joint_resources_p4_m1") return {std::make_unique<AgentModel<compositions::joint_resources_p4_m1::Agent>>()};
    if (name == "joint_resources_p4_m2") return {std::make_unique<AgentModel<compositions::joint_resources_p4_m2::Agent>>()};
    if (name == "joint_resources_p4_m3") return {std::make_unique<AgentModel<compositions::joint_resources_p4_m3::Agent>>()};
    if (name == "joint_resources_p4_m7") return {std::make_unique<AgentModel<compositions::joint_resources_p4_m7::Agent>>()};
    if (name == "joint_resources_p55_m0") return {std::make_unique<AgentModel<compositions::joint_resources_p55_m0::Agent>>()};
    if (name == "joint_resources_p55_m1") return {std::make_unique<AgentModel<compositions::joint_resources_p55_m1::Agent>>()};
    if (name == "joint_resources_p55_m2") return {std::make_unique<AgentModel<compositions::joint_resources_p55_m2::Agent>>()};
    if (name == "joint_resources_p55_m3") return {std::make_unique<AgentModel<compositions::joint_resources_p55_m3::Agent>>()};
    if (name == "joint_resources_p55_m7") return {std::make_unique<AgentModel<compositions::joint_resources_p55_m7::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
