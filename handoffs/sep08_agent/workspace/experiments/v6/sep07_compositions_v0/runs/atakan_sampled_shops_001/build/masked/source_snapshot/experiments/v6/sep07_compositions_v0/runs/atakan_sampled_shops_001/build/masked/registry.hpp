#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/source/audit.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router_v5/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_cow/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_sheep/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_goose/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_demand/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_quotes/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_value_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_portfolio_001/proposals/atakan_value_margin/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s0_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s0_margin/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s1_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s1_margin/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s8_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s8_margin/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s32_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s32_margin/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s64_own/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/atakan_sampled_shops_001/proposals/atakan_integrated_s64_margin/source/agent.hpp"
struct AnyAgent {virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
if(name=="pass")return {std::make_unique<Model<compositions::Pass>>()};
if(name=="animal_adaptive_r1_c0_b0")return {std::make_unique<Model<compositions::animal_adaptive_r1_c0_b0::Agent>>()};
if(name=="teammate_shoprouter")return {std::make_unique<Model<compositions::teammate_shoprouter::Agent>>()};
if(name=="public_router_v5")return {std::make_unique<Model<compositions::public_router_v5::Agent>>()};
if(name=="king_rc4")return {std::make_unique<Model<compositions::king_rc4::Agent>>()};
if(name=="public_router")return {std::make_unique<Model<compositions::public_router::Agent>>()};
if(name=="atakan_cow")return {std::make_unique<Model<compositions::atakan_cow::Agent>>()};
if(name=="atakan_sheep")return {std::make_unique<Model<compositions::atakan_sheep::Agent>>()};
if(name=="atakan_goose")return {std::make_unique<Model<compositions::atakan_goose::Agent>>()};
if(name=="atakan_demand")return {std::make_unique<Model<compositions::atakan_demand::Agent>>()};
if(name=="atakan_quotes")return {std::make_unique<Model<compositions::atakan_quotes::Agent>>()};
if(name=="atakan_value_own")return {std::make_unique<Model<compositions::atakan_value_own::Agent>>()};
if(name=="atakan_value_margin")return {std::make_unique<Model<compositions::atakan_value_margin::Agent>>()};
if(name=="atakan_integrated_s0_own")return {std::make_unique<Model<compositions::atakan_integrated_s0_own::Agent>>()};
if(name=="atakan_integrated_s0_margin")return {std::make_unique<Model<compositions::atakan_integrated_s0_margin::Agent>>()};
if(name=="atakan_integrated_s1_own")return {std::make_unique<Model<compositions::atakan_integrated_s1_own::Agent>>()};
if(name=="atakan_integrated_s1_margin")return {std::make_unique<Model<compositions::atakan_integrated_s1_margin::Agent>>()};
if(name=="atakan_integrated_s8_own")return {std::make_unique<Model<compositions::atakan_integrated_s8_own::Agent>>()};
if(name=="atakan_integrated_s8_margin")return {std::make_unique<Model<compositions::atakan_integrated_s8_margin::Agent>>()};
if(name=="atakan_integrated_s32_own")return {std::make_unique<Model<compositions::atakan_integrated_s32_own::Agent>>()};
if(name=="atakan_integrated_s32_margin")return {std::make_unique<Model<compositions::atakan_integrated_s32_margin::Agent>>()};
if(name=="atakan_integrated_s64_own")return {std::make_unique<Model<compositions::atakan_integrated_s64_own::Agent>>()};
if(name=="atakan_integrated_s64_margin")return {std::make_unique<Model<compositions::atakan_integrated_s64_margin::Agent>>()};
if(name=="investment_context_guarded_001_best")return {std::make_unique<Model<compositions::investment_context_guarded_001_best::Agent>>()};
std::abort();}
