#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_fert/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_plain/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_three_fert/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_three_plain/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_fert_unclosed/source/agent.hpp"
struct AnyAgent {uint32_t matched=0;bool berry=false;virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent{T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);matched=0;berry=false;}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);if constexpr(requires{value.matched_days();value.berry_selected();}){matched=value.matched_days();berry=value.berry_selected();}}};
struct Box{std::unique_ptr<AnyAgent>value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string&name){
if(name=="pass")return {std::make_unique<Model<compositions::Pass>>()};
if(name=="crop_value_m2_t4")return {std::make_unique<Model<compositions::crop_value_m2_t4::Agent>>()};
if(name=="teammate_shoprouter")return {std::make_unique<Model<compositions::teammate_shoprouter::Agent>>()};
if(name=="public_router_v5")return {std::make_unique<Model<compositions::public_router_v5::Agent>>()};
if(name=="king_rc4")return {std::make_unique<Model<compositions::king_rc4::Agent>>()};
if(name=="public_router")return {std::make_unique<Model<compositions::public_router::Agent>>()};
if(name=="wheat_one_fert")return {std::make_unique<Model<compositions::wheat_one_fert::Agent>>()};
if(name=="wheat_one_plain")return {std::make_unique<Model<compositions::wheat_one_plain::Agent>>()};
if(name=="wheat_three_fert")return {std::make_unique<Model<compositions::wheat_three_fert::Agent>>()};
if(name=="wheat_three_plain")return {std::make_unique<Model<compositions::wheat_three_plain::Agent>>()};
if(name=="wheat_one_fert_unclosed")return {std::make_unique<Model<compositions::wheat_one_fert_unclosed::Agent>>()};
std::abort();}
