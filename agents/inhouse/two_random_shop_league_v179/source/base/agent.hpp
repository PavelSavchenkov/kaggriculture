#pragma once

#include <array>
#include <cstdint>

#include "agents/common/api/agent_api.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/agent.hpp"

namespace kag::agents::two_random_shop_league_v179::base {

struct ReplanParameters {
    std::array<uint8_t, kag::N_PRODUCTS> reserve_per_demand = {
        0, 0, 0, 0, 0, 0, 2, 0, 0};
    std::array<uint16_t, kag::N_PRODUCTS> release_step = {
        719, 719, 719, 719, 719, 719, 288, 719, 719};
    std::array<uint8_t, kag::N_SHOPS> branch_delegate = {
        kag::SHOP_BAKERY, kag::SHOP_ICE_CREAM_SHOP,
        kag::SHOP_ICE_CREAM_SHOP,
        kag::SHOP_ICE_CREAM_SHOP, kag::SHOP_PET_CAFE, kag::SHOP_PIZZA_SHOP,
        kag::SHOP_SMOOTHIE_SHOP, kag::SHOP_YARN_STORE};
    uint8_t opening_first = kag::SHOP_BAKERY;
    uint8_t opening_second = kag::SHOP_BAKERY;
    uint8_t opening_cut_step = 72;
    uint8_t fallback_first_mask =
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET);
    uint8_t fallback_second_mask =
        (uint8_t{1} << kag::SHOP_BAKERY) |
        (uint8_t{1} << kag::SHOP_PET_CAFE) |
        (uint8_t{1} << kag::SHOP_YARN_STORE);
    uint16_t fallback_start_step = 144;
    uint16_t fallback_end_step = 287;
    uint8_t fallback_persistent_mask =
        uint8_t{1} << kag::SHOP_YARN_STORE;
    bool terminal_seed_repair = true;
    uint8_t minimum_headroom = 12;
    bool capacity_repair = true;
    uint8_t pizza_step695_second_milk = 0;
    uint8_t alternate_ice_first_mask =
        (uint8_t{1} << kag::SHOP_BAKERY) |
        (uint8_t{1} << kag::SHOP_PET_CAFE) |
        (uint8_t{1} << kag::SHOP_PIZZA_SHOP) |
        (uint8_t{1} << kag::SHOP_YARN_STORE);
    uint8_t alternate_ice_second_mask =
        (uint8_t{1} << kag::SHOP_ICE_CREAM_SHOP) |
        (uint8_t{1} << kag::SHOP_SMOOTHIE_SHOP);
    uint8_t alternate_ice_extra_first_mask =
        uint8_t{1} << kag::SHOP_PIZZA_SHOP;
    uint8_t alternate_ice_extra_second_mask =
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET);
    bool alternate_ice_seed_repair = true;
    uint16_t alternate_ice_start_step = 144;
    uint8_t alternate_yarn_first_mask =
        (uint8_t{1} << kag::SHOP_BAKERY) |
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET) |
        (uint8_t{1} << kag::SHOP_PET_CAFE) |
        (uint8_t{1} << kag::SHOP_PIZZA_SHOP);
    uint8_t alternate_yarn_second_mask =
        uint8_t{1} << kag::SHOP_YARN_STORE;
    uint16_t alternate_yarn_start_step = 144;
    uint8_t alternate_pizza_first_mask =
        (uint8_t{1} << kag::SHOP_BAKERY) |
        (uint8_t{1} << kag::SHOP_PET_CAFE);
    uint8_t alternate_pizza_second_mask =
        uint8_t{1} << kag::SHOP_PIZZA_SHOP;
    uint16_t alternate_pizza_start_step = 255;
    uint8_t alternate_brunch_first_mask =
        (uint8_t{1} << kag::SHOP_BAKERY) |
        (uint8_t{1} << kag::SHOP_PET_CAFE);
    uint8_t alternate_brunch_second_mask =
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET);
    uint16_t alternate_brunch_start_step = 216;
    uint16_t alternate_brunch_end_step = 720;
    uint8_t terminal_egg_headroom = 2;
};

class Agent {
public:
    static kag::agent::AgentInfo info();
    void set_parameters(const ReplanParameters& parameters);
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);
    int repair_activations() const { return state_.repair_activations; }

private:
    struct PolicyState {
        std::array<uint8_t, kag::N_SHOPS> instance_count{};
        std::array<uint16_t, kag::MAX_SHOP_INSTANCES> reveal_step{};
        std::array<uint16_t, kag::N_PRODUCTS> demand{};
        std::array<int16_t, kag::N_ITEMS> shed{};
        std::array<int16_t, kag::N_ITEMS> cargo{};
        std::array<int16_t, kag::N_CROPS> seeds{};
        std::array<int32_t, kag::N_PRODUCTS> market_inventory{};
        std::array<int32_t, kag::N_PRODUCTS> market_price{};
        std::array<uint8_t, kag::N_CROPS> growing_crops{};
        std::array<uint8_t, kag::N_ANIMALS> animals{};
        int cash = 0;
        int workers = 1;
        int shed_total = 0;
        int remaining_steps = 719;
        int seen_shops = 0;
        int repair_activations = 0;
    } state_;

    warm::one_random_shop_dynamic_robust::Agent warm_;
    warm::one_random_shop_dynamic_robust::Agent fallback_warm_;
    warm::one_random_shop_dynamic_robust::Agent alternate_ice_warm_;
    warm::one_random_shop_dynamic_robust::Agent alternate_yarn_warm_;
    warm::one_random_shop_dynamic_robust::Agent alternate_pizza_warm_;
    warm::one_random_shop_dynamic_robust::Agent alternate_brunch_warm_;
    ReplanParameters parameters_{};
    bool initialized_ = false;
    bool configured_ = false;

    void configure_warm_agents();
    void update_state(const kag::agent::AgentObservation& observation);
    void apply_second_reveal_replan(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_terminal_seeds(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_capacity(const kag::agent::AgentObservation& observation,
                         kag::Action& action);
    void repair_terminal_eggs(
        const kag::agent::AgentObservation& observation, kag::Action& action);
};

}  // namespace kag::agents::two_random_shop_league_v179::base
