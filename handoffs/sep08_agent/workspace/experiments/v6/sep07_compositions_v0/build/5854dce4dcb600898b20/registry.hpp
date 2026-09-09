#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/compiler_care_sep08_001/proposals/compiler_care_mixed_m1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p13/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p14/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p150/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p151/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p152/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p192/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p193/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p194/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p246/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p247/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p248/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p309/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p310/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p311/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p342/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p343/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p344/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p351/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p352/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p353/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p354/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p355/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p356/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p358/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p360/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p361/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p66/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p70/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p71/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/dated_expansion_sep08_001/proposals/dated_expansion_p9/source/agent.hpp"
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
    if (name == "dated_expansion_p13") return {std::make_unique<AgentModel<compositions::dated_expansion_p13::Agent>>()};
    if (name == "dated_expansion_p14") return {std::make_unique<AgentModel<compositions::dated_expansion_p14::Agent>>()};
    if (name == "dated_expansion_p150") return {std::make_unique<AgentModel<compositions::dated_expansion_p150::Agent>>()};
    if (name == "dated_expansion_p151") return {std::make_unique<AgentModel<compositions::dated_expansion_p151::Agent>>()};
    if (name == "dated_expansion_p152") return {std::make_unique<AgentModel<compositions::dated_expansion_p152::Agent>>()};
    if (name == "dated_expansion_p192") return {std::make_unique<AgentModel<compositions::dated_expansion_p192::Agent>>()};
    if (name == "dated_expansion_p193") return {std::make_unique<AgentModel<compositions::dated_expansion_p193::Agent>>()};
    if (name == "dated_expansion_p194") return {std::make_unique<AgentModel<compositions::dated_expansion_p194::Agent>>()};
    if (name == "dated_expansion_p2") return {std::make_unique<AgentModel<compositions::dated_expansion_p2::Agent>>()};
    if (name == "dated_expansion_p246") return {std::make_unique<AgentModel<compositions::dated_expansion_p246::Agent>>()};
    if (name == "dated_expansion_p247") return {std::make_unique<AgentModel<compositions::dated_expansion_p247::Agent>>()};
    if (name == "dated_expansion_p248") return {std::make_unique<AgentModel<compositions::dated_expansion_p248::Agent>>()};
    if (name == "dated_expansion_p309") return {std::make_unique<AgentModel<compositions::dated_expansion_p309::Agent>>()};
    if (name == "dated_expansion_p310") return {std::make_unique<AgentModel<compositions::dated_expansion_p310::Agent>>()};
    if (name == "dated_expansion_p311") return {std::make_unique<AgentModel<compositions::dated_expansion_p311::Agent>>()};
    if (name == "dated_expansion_p342") return {std::make_unique<AgentModel<compositions::dated_expansion_p342::Agent>>()};
    if (name == "dated_expansion_p343") return {std::make_unique<AgentModel<compositions::dated_expansion_p343::Agent>>()};
    if (name == "dated_expansion_p344") return {std::make_unique<AgentModel<compositions::dated_expansion_p344::Agent>>()};
    if (name == "dated_expansion_p351") return {std::make_unique<AgentModel<compositions::dated_expansion_p351::Agent>>()};
    if (name == "dated_expansion_p352") return {std::make_unique<AgentModel<compositions::dated_expansion_p352::Agent>>()};
    if (name == "dated_expansion_p353") return {std::make_unique<AgentModel<compositions::dated_expansion_p353::Agent>>()};
    if (name == "dated_expansion_p354") return {std::make_unique<AgentModel<compositions::dated_expansion_p354::Agent>>()};
    if (name == "dated_expansion_p355") return {std::make_unique<AgentModel<compositions::dated_expansion_p355::Agent>>()};
    if (name == "dated_expansion_p356") return {std::make_unique<AgentModel<compositions::dated_expansion_p356::Agent>>()};
    if (name == "dated_expansion_p358") return {std::make_unique<AgentModel<compositions::dated_expansion_p358::Agent>>()};
    if (name == "dated_expansion_p360") return {std::make_unique<AgentModel<compositions::dated_expansion_p360::Agent>>()};
    if (name == "dated_expansion_p361") return {std::make_unique<AgentModel<compositions::dated_expansion_p361::Agent>>()};
    if (name == "dated_expansion_p362") return {std::make_unique<AgentModel<compositions::dated_expansion_p362::Agent>>()};
    if (name == "dated_expansion_p66") return {std::make_unique<AgentModel<compositions::dated_expansion_p66::Agent>>()};
    if (name == "dated_expansion_p70") return {std::make_unique<AgentModel<compositions::dated_expansion_p70::Agent>>()};
    if (name == "dated_expansion_p71") return {std::make_unique<AgentModel<compositions::dated_expansion_p71::Agent>>()};
    if (name == "dated_expansion_p9") return {std::make_unique<AgentModel<compositions::dated_expansion_p9::Agent>>()};
    if (name == "observed_sale_lead_start_216") return {std::make_unique<AgentModel<kag::agents::observed_sale_lead_start_216::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    std::abort();
}
