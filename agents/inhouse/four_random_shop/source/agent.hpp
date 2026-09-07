#pragma once

#include <memory>

#include "agents/common/api/agent_api.hpp"
#include "agents/inhouse/four_random_shop/source/base/agent.hpp"

namespace kag::agents::four_random_shop {

struct Parameters {
    uint8_t milk_reserve_per_demand = 1;
    uint16_t milk_release_step = 336;
    uint8_t forced_mode = kag::SHOP_ICE_CREAM_SHOP;
    uint16_t forced_mode_start_step = 239;
    uint8_t mode_rule = 31;
    bool terminal_cargo_repair = true;
    uint8_t terminal_seed_mask = 33;
    uint8_t third_yarn_rule = 2;
    uint8_t third_pizza_rule = 2;
    uint8_t fourth_pizza_rule = 16;
    uint8_t fourth_yarn_rule = 0;
    uint16_t third_yarn_start_step = 216;
    uint16_t third_pizza_start_step = 264;
    uint8_t minimum_headroom = 0;
    uint8_t terminal_egg_headroom = 2;
    uint8_t pizza_step695_second_milk = 0;
    uint8_t smoothie_rule = 1;
    uint8_t opening_fertilizer_sale = 4;
    bool pet_reveal_wheat = true;
    bool flexible_animal = false;
    uint8_t pet_day11_melon_sale = 255;
    uint8_t pet_day11_wheat_shift = 0;
};

class Agent {
public:
    static kag::agent::AgentInfo info();
    void set_parameters(const Parameters& parameters);
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);
    int repair_activations() const {
        return base_.repair_activations() + repair_activations_;
    }

private:
    kag::agents::four_random_shop::base::Agent base_;
    kag::agents::four_random_shop::base::Agent mode_;
    kag::agents::four_random_shop::base::Agent yarn_;
    kag::agents::four_random_shop::base::Agent pizza_;
    std::unique_ptr<kag::agents::four_random_shop::base::Agent> smoothie_;
    Parameters parameters_{};
    bool initialized_ = false;
    bool configured_ = false;
    int repair_activations_ = 0;

    void configure();
    bool use_mode(const kag::agent::AgentObservation& observation) const;
    bool use_yarn(const kag::agent::AgentObservation& observation) const;
    bool use_pizza(const kag::agent::AgentObservation& observation) const;
    bool use_smoothie(const kag::agent::AgentObservation& observation) const;
    void repair_unaffordable_reveal_buy(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_day_end_capacity(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_day_end_cargo_capacity(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_terminal_cargo(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void repair_terminal_seed_buys(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void adjust_pet_day11_melon_sale(
        const kag::agent::AgentObservation& observation, kag::Action& action);
    void adjust_pet_wheat_timing(
        const kag::agent::AgentObservation& observation, kag::Action& action);
};

}  // namespace kag::agents::four_random_shop
