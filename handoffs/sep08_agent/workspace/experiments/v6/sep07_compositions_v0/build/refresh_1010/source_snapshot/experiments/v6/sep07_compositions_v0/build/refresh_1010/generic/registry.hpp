#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_terminal_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/binghua_116/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/titan_frontier/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/kaito_v43/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/market_impact_v4/source/agent.hpp"
struct AnyAgent {virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
if(name=="pass")return {std::make_unique<Model<compositions::Pass>>()};
if(name=="public_router")return {std::make_unique<Model<compositions::public_router::Agent>>()};
if(name=="public_terminal_router")return {std::make_unique<Model<compositions::public_terminal_router::Agent>>()};
if(name=="king_rc4")return {std::make_unique<Model<compositions::king_rc4::Agent>>()};
if(name=="teammate_shoprouter")return {std::make_unique<Model<compositions::teammate_shoprouter::Agent>>()};
if(name=="shop_herd_s6_m3_g1")return {std::make_unique<Model<compositions::shop_herd_s6_m3_g1::Agent>>()};
if(name=="binghua_116")return {std::make_unique<Model<compositions::binghua_116::Agent>>()};
if(name=="titan_frontier")return {std::make_unique<Model<compositions::titan_frontier::Agent>>()};
if(name=="kaito_v43")return {std::make_unique<Model<compositions::kaito_v43::Agent>>()};
if(name=="market_impact_v4")return {std::make_unique<Model<compositions::market_impact_v4::Agent>>()};
std::abort();}
