#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/sheep_expansion_portfolio_001/source/audit.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/sheep_expansion_portfolio_001/proposals/sheep_fixed_small_terminal/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/sheep_expansion_portfolio_001/proposals/sheep_fixed_expansion_terminal/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/sheep_expansion_portfolio_001/proposals/sheep_yarn2_terminal/source/agent.hpp"
struct Trace {int branch=-1,wheat=0;double cash=0,rival_cash=0;std::array<compositions::sheep_portfolio::Estimate,2> estimates{};std::vector<int> physical;};
struct AnyAgent {Trace trace;virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);trace={};}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);if constexpr(requires{value.branch();value.estimates();})if(o.step==288){trace.branch=value.branch();trace.estimates=value.estimates();trace.cash=o.self().money;trace.rival_cash=o.opponent().money;trace.wheat=o.own.shed[kag::WHEAT];trace.physical=compositions::sheep_portfolio::physical(o);}}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
if(name=="pass")return {std::make_unique<Model<compositions::Pass>>()};
if(name=="investment_context_guarded_001_best")return {std::make_unique<Model<compositions::investment_context_guarded_001_best::Agent>>()};
if(name=="teammate_shoprouter")return {std::make_unique<Model<compositions::teammate_shoprouter::Agent>>()};
if(name=="public_router_v5")return {std::make_unique<Model<compositions::public_router_v5::Agent>>()};
if(name=="king_rc4")return {std::make_unique<Model<compositions::king_rc4::Agent>>()};
if(name=="public_router")return {std::make_unique<Model<compositions::public_router::Agent>>()};
if(name=="sheep_fixed_small_terminal")return {std::make_unique<Model<compositions::sheep_fixed_small_terminal::Agent>>()};
if(name=="sheep_fixed_expansion_terminal")return {std::make_unique<Model<compositions::sheep_fixed_expansion_terminal::Agent>>()};
if(name=="sheep_yarn2_terminal")return {std::make_unique<Model<compositions::sheep_yarn2_terminal::Agent>>()};
std::abort();}
