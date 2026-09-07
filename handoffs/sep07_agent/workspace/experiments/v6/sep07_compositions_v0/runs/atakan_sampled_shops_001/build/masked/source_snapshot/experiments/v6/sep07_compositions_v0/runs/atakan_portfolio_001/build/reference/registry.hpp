#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/source/audit.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_cow/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_sheep/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_goose/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_demand/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_quotes/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_value_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_value_margin/source/agent.hpp"
struct Trace {int branch=-1,wheat=0;double cash=0,rival_cash=0;std::array<compositions::atakan_portfolio::Estimate,3> estimates{};std::vector<int> physical;};
struct AnyAgent {Trace trace;virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);trace={};}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);if constexpr(requires{value.branch();value.estimates();})if(o.step==226){trace.branch=value.branch();trace.estimates=value.estimates();trace.cash=o.self().money;trace.rival_cash=o.opponent().money;trace.wheat=o.own.shed[kag::WHEAT];trace.physical=compositions::atakan_portfolio::physical(o);}}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
if(name=="pass")return {std::make_unique<Model<compositions::Pass>>()};
if(name=="investment_context_guarded_001_best")return {std::make_unique<Model<compositions::investment_context_guarded_001_best::Agent>>()};
if(name=="atakan_cow")return {std::make_unique<Model<compositions::atakan_cow::Agent>>()};
if(name=="atakan_sheep")return {std::make_unique<Model<compositions::atakan_sheep::Agent>>()};
if(name=="atakan_goose")return {std::make_unique<Model<compositions::atakan_goose::Agent>>()};
if(name=="atakan_demand")return {std::make_unique<Model<compositions::atakan_demand::Agent>>()};
if(name=="atakan_quotes")return {std::make_unique<Model<compositions::atakan_quotes::Agent>>()};
if(name=="atakan_value_own")return {std::make_unique<Model<compositions::atakan_value_own::Agent>>()};
if(name=="atakan_value_margin")return {std::make_unique<Model<compositions::atakan_value_margin::Agent>>()};
std::abort();}
