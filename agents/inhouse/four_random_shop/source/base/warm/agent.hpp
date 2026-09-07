#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "agents/common/api/agent_api.hpp"

namespace kag::agents::four_random_shop::base::warm::one_random_shop_dynamic_robust {

class Agent {
public:
    Agent();
    ~Agent();
    Agent(Agent&&) noexcept;
    Agent& operator=(Agent&&) noexcept;
    Agent(const Agent&) = delete;
    Agent& operator=(const Agent&) = delete;

    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action);

    void set_opening_shop(int shop);
    void set_opening_schedule(int first_shop, int second_shop, int cut_step);
    void set_opening_sources(const std::array<uint8_t, 72>& sources);
    void set_branch_shop(int observed_shop, int delegate_shop);
    void set_shadow_rejoin(bool enabled);
    void set_flexible_animal(bool enabled);
    void set_opening_fertilizer_sale(int quantity);
    void set_yarn_repair(bool enabled);
    void set_pizza_reveal_wheat(int quantity);
    void set_pet_reveal_wheat(bool enabled);
    void set_pizza_day7_fertilizer(int quantity);
    void set_pizza_day12_wheat(int quantity);
    void set_pizza_step383_milk(int quantity);
    void set_pizza_day20_wheat_sale(int quantity);
    void set_pizza_step695_second_milk(int quantity);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kag::agents::four_random_shop::base::warm::one_random_shop_dynamic_robust
