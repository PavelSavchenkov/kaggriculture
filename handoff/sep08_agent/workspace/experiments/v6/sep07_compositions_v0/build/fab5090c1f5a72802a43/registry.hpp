#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_care_sep08_001/proposals/compiler_care_mixed_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p107/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p147/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p148/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p149/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p210/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p225/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p228/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p229/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p230/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p264/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p265/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p266/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p30/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p309/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p310/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p311/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p342/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p343/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p344/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p355/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p356/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p361/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p363/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p364/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p365/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p49/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p50/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p90/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_002/proposals/dated_expansion_p91/source/agent.hpp"
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
    if (name == "compiler_care_mixed_m1") return {std::make_unique<AgentModel<compositions::compiler_care_mixed_m1::Agent>>()};
    if (name == "dated_expansion_p0") return {std::make_unique<AgentModel<compositions::dated_expansion_p0::Agent>>()};
    if (name == "dated_expansion_p1") return {std::make_unique<AgentModel<compositions::dated_expansion_p1::Agent>>()};
    if (name == "dated_expansion_p107") return {std::make_unique<AgentModel<compositions::dated_expansion_p107::Agent>>()};
    if (name == "dated_expansion_p147") return {std::make_unique<AgentModel<compositions::dated_expansion_p147::Agent>>()};
    if (name == "dated_expansion_p148") return {std::make_unique<AgentModel<compositions::dated_expansion_p148::Agent>>()};
    if (name == "dated_expansion_p149") return {std::make_unique<AgentModel<compositions::dated_expansion_p149::Agent>>()};
    if (name == "dated_expansion_p2") return {std::make_unique<AgentModel<compositions::dated_expansion_p2::Agent>>()};
    if (name == "dated_expansion_p210") return {std::make_unique<AgentModel<compositions::dated_expansion_p210::Agent>>()};
    if (name == "dated_expansion_p225") return {std::make_unique<AgentModel<compositions::dated_expansion_p225::Agent>>()};
    if (name == "dated_expansion_p228") return {std::make_unique<AgentModel<compositions::dated_expansion_p228::Agent>>()};
    if (name == "dated_expansion_p229") return {std::make_unique<AgentModel<compositions::dated_expansion_p229::Agent>>()};
    if (name == "dated_expansion_p230") return {std::make_unique<AgentModel<compositions::dated_expansion_p230::Agent>>()};
    if (name == "dated_expansion_p264") return {std::make_unique<AgentModel<compositions::dated_expansion_p264::Agent>>()};
    if (name == "dated_expansion_p265") return {std::make_unique<AgentModel<compositions::dated_expansion_p265::Agent>>()};
    if (name == "dated_expansion_p266") return {std::make_unique<AgentModel<compositions::dated_expansion_p266::Agent>>()};
    if (name == "dated_expansion_p30") return {std::make_unique<AgentModel<compositions::dated_expansion_p30::Agent>>()};
    if (name == "dated_expansion_p309") return {std::make_unique<AgentModel<compositions::dated_expansion_p309::Agent>>()};
    if (name == "dated_expansion_p310") return {std::make_unique<AgentModel<compositions::dated_expansion_p310::Agent>>()};
    if (name == "dated_expansion_p311") return {std::make_unique<AgentModel<compositions::dated_expansion_p311::Agent>>()};
    if (name == "dated_expansion_p342") return {std::make_unique<AgentModel<compositions::dated_expansion_p342::Agent>>()};
    if (name == "dated_expansion_p343") return {std::make_unique<AgentModel<compositions::dated_expansion_p343::Agent>>()};
    if (name == "dated_expansion_p344") return {std::make_unique<AgentModel<compositions::dated_expansion_p344::Agent>>()};
    if (name == "dated_expansion_p355") return {std::make_unique<AgentModel<compositions::dated_expansion_p355::Agent>>()};
    if (name == "dated_expansion_p356") return {std::make_unique<AgentModel<compositions::dated_expansion_p356::Agent>>()};
    if (name == "dated_expansion_p361") return {std::make_unique<AgentModel<compositions::dated_expansion_p361::Agent>>()};
    if (name == "dated_expansion_p362") return {std::make_unique<AgentModel<compositions::dated_expansion_p362::Agent>>()};
    if (name == "dated_expansion_p363") return {std::make_unique<AgentModel<compositions::dated_expansion_p363::Agent>>()};
    if (name == "dated_expansion_p364") return {std::make_unique<AgentModel<compositions::dated_expansion_p364::Agent>>()};
    if (name == "dated_expansion_p365") return {std::make_unique<AgentModel<compositions::dated_expansion_p365::Agent>>()};
    if (name == "dated_expansion_p49") return {std::make_unique<AgentModel<compositions::dated_expansion_p49::Agent>>()};
    if (name == "dated_expansion_p50") return {std::make_unique<AgentModel<compositions::dated_expansion_p50::Agent>>()};
    if (name == "dated_expansion_p90") return {std::make_unique<AgentModel<compositions::dated_expansion_p90::Agent>>()};
    if (name == "dated_expansion_p91") return {std::make_unique<AgentModel<compositions::dated_expansion_p91::Agent>>()};
    if (name == "observed_sale_lead_start_216") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_start_216::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
