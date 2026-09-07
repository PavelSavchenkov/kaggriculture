#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "agents/external/external_replay_band_adamjonesjohnson_89413383_safe/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/arman_3000/source/agent.hpp"
#include "agents/external/boatlee_h7_fast_sanitize/source/agent.hpp"
#include "agents/external/c68_h18_fast_sanitize/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_greedy_v0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_input_guard/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_reserve3/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_source_guard/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_source_service/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/crop_dusta_3000/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/deniz/source/agent.hpp"
#include "agents/external/public_indar_e279_unchanged/source/agent.hpp"
#include "agents/external/external_replay_band_jeff_horon_89417087_safe/source/agent.hpp"
#include "agents/external/public_kaito_h10_front_run_fast/source/agent.hpp"
#include "agents/external/external_replay_band_md_mehedi_hasan_89415485_safe/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/mrkiwi_3000/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "agents/external/public_roman_hamburger_anchor_unchanged/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/ryo_3000/source/agent.hpp"
#include "agents/external/public_skomuro_2000_cpp/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/structured/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/subramanya_3000/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "agents/external/external_replay_band_xdang13_89917554_safe/source/agent.hpp"
#include "agents/external/external_replay_band_yaroslav_tanko_89416002_safe/source/agent.hpp"
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
    if (name == "adam_safe") return {std::make_unique<AgentModel<four_shop_search::external_replay_band_adamjonesjohnson_89413383_safe::Agent>>()};
    if (name == "arman_3000") return {std::make_unique<AgentModel<four_shop_foundry::replay::replay_arman_ge3000_94541153::Agent>>()};
    if (name == "boatlee") return {std::make_unique<AgentModel<four_shop_foundry::throughput::boatlee_h7_fast_sanitize::Agent>>()};
    if (name == "c68") return {std::make_unique<AgentModel<four_shop_foundry::throughput::c68_h18_fast_sanitize::Agent>>()};
    if (name == "composition_greedy_v0") return {std::make_unique<AgentModel<compositions::greedy::Agent<0>>>()};
    if (name == "composition_input_guard") return {std::make_unique<AgentModel<compositions::greedy::Agent<0,true,true,false,1>>>()};
    if (name == "composition_reserve3") return {std::make_unique<AgentModel<compositions::greedy::Agent<0,true,true,false,3>>>()};
    if (name == "composition_source_guard") return {std::make_unique<AgentModel<compositions::greedy::Agent<0,true,true,true,1>>>()};
    if (name == "composition_source_service") return {std::make_unique<AgentModel<compositions::greedy::Agent<0,true,true,true>>>()};
    if (name == "crop_dusta_3000") return {std::make_unique<AgentModel<four_shop_foundry::replay::replay_crop_dusta_ge3000_100223989::Agent>>()};
    if (name == "deniz") return {std::make_unique<AgentModel<four_shop_foundry::public_adapted::deniz_v111_safe::Agent>>()};
    if (name == "indar") return {std::make_unique<AgentModel<league::public_unchanged::indar::Policy>>()};
    if (name == "jeff_safe") return {std::make_unique<AgentModel<four_shop_search::external_replay_band_jeff_horon_89417087_safe::Agent>>()};
    if (name == "kaito") return {std::make_unique<AgentModel<league::public_optimized::kaito_h10_front_run_fast::Policy>>()};
    if (name == "mehedi_safe") return {std::make_unique<AgentModel<four_shop_search::external_replay_band_md_mehedi_hasan_89415485_safe::Agent>>()};
    if (name == "mrkiwi_3000") return {std::make_unique<AgentModel<four_shop_foundry::replay::test_replay_mrkiwi_ge3000_93167917::Agent>>()};
    if (name == "public_router") return {std::make_unique<AgentModel<compositions::public_router::Agent>>()};
    if (name == "roman") return {std::make_unique<AgentModel<league::public_unchanged::roman_hamburger::Policy>>()};
    if (name == "ryo_3000") return {std::make_unique<AgentModel<four_shop_foundry::replay::replay_ryo_ge3000_95029942::Agent>>()};
    if (name == "skomuro") return {std::make_unique<AgentModel<four_shop_search::public_skomuro_2000_cpp::Agent>>()};
    if (name == "structured") return {std::make_unique<AgentModel<league::test::structured_economic_policy::Policy>>()};
    if (name == "subramanya_3000") return {std::make_unique<AgentModel<four_shop_foundry::replay::replay_subramanya_ge3000_96594837::Agent>>()};
    if (name == "teammate_shoprouter") return {std::make_unique<AgentModel<compositions::teammate_shoprouter::Agent>>()};
    if (name == "xdang_safe") return {std::make_unique<AgentModel<four_shop_search::external_replay_band_xdang13_89917554_safe::Agent>>()};
    if (name == "yaroslav_safe") return {std::make_unique<AgentModel<four_shop_search::external_replay_band_yaroslav_tanko_89416002_safe::Agent>>()};
    std::abort();
}
