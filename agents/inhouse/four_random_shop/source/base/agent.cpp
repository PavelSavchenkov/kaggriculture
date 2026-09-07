#include "agents/inhouse/four_random_shop/source/base/agent.hpp"

#include <algorithm>
#include <cstdlib>

namespace kag::agents::four_random_shop::base {
namespace {

constexpr std::array<std::array<uint8_t, kag::N_PRODUCTS>, kag::N_SHOPS>
    SHOP_DEMAND = {{
        {{1, 0, 0, 0, 0, 1, 0, 0}},
        {{1, 0, 0, 1, 0, 1, 0, 0}},
        {{1, 1, 1, 1, 0, 0, 0, 0}},
        {{1, 0, 0, 1, 0, 0, 1, 0}},
        {{0, 2, 0, 0, 0, 0, 0, 0}},
        {{1, 0, 1, 0, 0, 0, 1, 0}},
        {{0, 0, 0, 1, 0, 0, 1, 0}},
        {{0, 0, 0, 0, 0, 0, 0, 2}},
    }};

}  // namespace

kag::agent::AgentInfo Agent::info() { return {"two_random_shop"}; }

void Agent::set_parameters(const ReplanParameters& parameters) {
    if (initialized_) std::abort();
    parameters_ = parameters;
    for (int product = 0; product < kag::N_PRODUCTS; ++product)
        if (parameters_.release_step[product] > 719) std::abort();
    if (parameters_.fallback_start_step < 144 ||
        parameters_.fallback_end_step > 720 ||
        parameters_.fallback_start_step >= parameters_.fallback_end_step)
        std::abort();
    if (parameters_.alternate_ice_start_step < 144 ||
        parameters_.alternate_ice_start_step > 719 ||
        parameters_.alternate_yarn_start_step < 144 ||
        parameters_.alternate_yarn_start_step > 719 ||
        parameters_.alternate_pizza_start_step < 144 ||
        parameters_.alternate_pizza_start_step > 719 ||
        parameters_.alternate_brunch_start_step < 144 ||
        parameters_.alternate_brunch_end_step > 720 ||
        parameters_.alternate_brunch_start_step >=
            parameters_.alternate_brunch_end_step)
        std::abort();
    if (!parameters_.terminal_egg_headroom ||
        parameters_.terminal_egg_headroom > 20)
        std::abort();
    if (parameters_.fallback_persistent_mask &
        ~parameters_.fallback_second_mask)
        std::abort();
    if (parameters_.opening_first >= kag::N_SHOPS ||
        parameters_.opening_second >= kag::N_SHOPS ||
        parameters_.opening_cut_step > 72 ||
        parameters_.pizza_step695_second_milk > 50 ||
        parameters_.opening_fertilizer_sale > 20)
        std::abort();
    configure_warm_agents();
}

void Agent::configure_warm_agents() {
    if (configured_) std::abort();
    warm_.set_opening_schedule(parameters_.opening_first,
                               parameters_.opening_second,
                               parameters_.opening_cut_step);
    warm_.set_pizza_step695_second_milk(
        parameters_.pizza_step695_second_milk);
    warm_.set_opening_fertilizer_sale(parameters_.opening_fertilizer_sale);
    warm_.set_pet_reveal_wheat(parameters_.pet_reveal_wheat);
    warm_.set_flexible_animal(parameters_.flexible_animal);
    for (int shop = 0; shop < kag::N_SHOPS; ++shop) {
        if (parameters_.branch_delegate[shop] >= kag::N_SHOPS) std::abort();
        if (parameters_.branch_delegate[shop] != shop)
            warm_.set_branch_shop(shop, parameters_.branch_delegate[shop]);
    }
    if (parameters_.fallback_first_mask && parameters_.fallback_second_mask) {
        fallback_warm_ = std::make_unique<
            warm::one_random_shop_dynamic_robust::Agent>();
        fallback_warm_->set_opening_schedule(parameters_.opening_first,
                                             parameters_.opening_second,
                                             parameters_.opening_cut_step);
        fallback_warm_->set_pizza_step695_second_milk(
            parameters_.pizza_step695_second_milk);
        fallback_warm_->set_opening_fertilizer_sale(
            parameters_.opening_fertilizer_sale);
        fallback_warm_->set_pet_reveal_wheat(parameters_.pet_reveal_wheat);
        fallback_warm_->set_flexible_animal(parameters_.flexible_animal);
    }
    if ((parameters_.alternate_ice_first_mask &&
         parameters_.alternate_ice_second_mask) ||
        (parameters_.alternate_ice_extra_first_mask &&
         parameters_.alternate_ice_extra_second_mask)) {
        alternate_ice_warm_ = std::make_unique<
            warm::one_random_shop_dynamic_robust::Agent>();
        alternate_ice_warm_->set_opening_schedule(parameters_.opening_first,
                                                  parameters_.opening_second,
                                                  parameters_.opening_cut_step);
        for (int shop = 0; shop < kag::N_SHOPS; ++shop)
            if (shop != kag::SHOP_ICE_CREAM_SHOP)
                alternate_ice_warm_->set_branch_shop(
                    shop, kag::SHOP_ICE_CREAM_SHOP);
        alternate_ice_warm_->set_pizza_step695_second_milk(
            parameters_.pizza_step695_second_milk);
        alternate_ice_warm_->set_opening_fertilizer_sale(
            parameters_.opening_fertilizer_sale);
        alternate_ice_warm_->set_pet_reveal_wheat(
            parameters_.pet_reveal_wheat);
        alternate_ice_warm_->set_flexible_animal(
            parameters_.flexible_animal);
    }
    if (parameters_.alternate_yarn_first_mask &&
        parameters_.alternate_yarn_second_mask) {
        alternate_yarn_warm_ = std::make_unique<
            warm::one_random_shop_dynamic_robust::Agent>();
        alternate_yarn_warm_->set_opening_schedule(parameters_.opening_first,
                                                   parameters_.opening_second,
                                                   parameters_.opening_cut_step);
        for (int shop = 0; shop < kag::N_SHOPS; ++shop)
            if (shop != kag::SHOP_YARN_STORE)
                alternate_yarn_warm_->set_branch_shop(
                    shop, kag::SHOP_YARN_STORE);
        alternate_yarn_warm_->set_pizza_step695_second_milk(
            parameters_.pizza_step695_second_milk);
        alternate_yarn_warm_->set_opening_fertilizer_sale(
            parameters_.opening_fertilizer_sale);
        alternate_yarn_warm_->set_pet_reveal_wheat(
            parameters_.pet_reveal_wheat);
        alternate_yarn_warm_->set_flexible_animal(
            parameters_.flexible_animal);
    }
    if (parameters_.alternate_pizza_first_mask &&
        parameters_.alternate_pizza_second_mask) {
        alternate_pizza_warm_ = std::make_unique<
            warm::one_random_shop_dynamic_robust::Agent>();
        alternate_pizza_warm_->set_opening_schedule(parameters_.opening_first,
                                                    parameters_.opening_second,
                                                    parameters_.opening_cut_step);
        for (int shop = 0; shop < kag::N_SHOPS; ++shop)
            if (shop != kag::SHOP_PIZZA_SHOP)
                alternate_pizza_warm_->set_branch_shop(
                    shop, kag::SHOP_PIZZA_SHOP);
        alternate_pizza_warm_->set_pizza_step695_second_milk(
            parameters_.pizza_step695_second_milk);
        alternate_pizza_warm_->set_opening_fertilizer_sale(
            parameters_.opening_fertilizer_sale);
        alternate_pizza_warm_->set_pet_reveal_wheat(
            parameters_.pet_reveal_wheat);
        alternate_pizza_warm_->set_flexible_animal(
            parameters_.flexible_animal);
    }
    if (parameters_.alternate_brunch_first_mask &&
        parameters_.alternate_brunch_second_mask) {
        alternate_brunch_warm_ = std::make_unique<
            warm::one_random_shop_dynamic_robust::Agent>();
        alternate_brunch_warm_->set_opening_schedule(
        parameters_.opening_first, parameters_.opening_second,
            parameters_.opening_cut_step);
        for (int shop = 0; shop < kag::N_SHOPS; ++shop)
            if (shop != kag::SHOP_BRUNCH_SPOT)
                alternate_brunch_warm_->set_branch_shop(
                    shop, kag::SHOP_BRUNCH_SPOT);
        alternate_brunch_warm_->set_pizza_step695_second_milk(
            parameters_.pizza_step695_second_milk);
        alternate_brunch_warm_->set_opening_fertilizer_sale(
            parameters_.opening_fertilizer_sale);
        alternate_brunch_warm_->set_pet_reveal_wheat(
            parameters_.pet_reveal_wheat);
        alternate_brunch_warm_->set_flexible_animal(
            parameters_.flexible_animal);
    }
    configured_ = true;
}

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!configured_) configure_warm_agents();
    state_ = {};
    state_.remaining_steps = init.config.episode_steps - 1;
    warm_.reset(init);
    if (parameters_.fallback_first_mask && parameters_.fallback_second_mask)
        fallback_warm_->reset(init);
    if ((parameters_.alternate_ice_first_mask &&
         parameters_.alternate_ice_second_mask) ||
        (parameters_.alternate_ice_extra_first_mask &&
         parameters_.alternate_ice_extra_second_mask))
        alternate_ice_warm_->reset(init);
    if (parameters_.alternate_yarn_first_mask &&
        parameters_.alternate_yarn_second_mask)
        alternate_yarn_warm_->reset(init);
    if (parameters_.alternate_pizza_first_mask &&
        parameters_.alternate_pizza_second_mask)
        alternate_pizza_warm_->reset(init);
    if (parameters_.alternate_brunch_first_mask &&
        parameters_.alternate_brunch_second_mask)
        alternate_brunch_warm_->reset(init);
    initialized_ = true;
}

void Agent::update_state(
    const kag::agent::AgentObservation& observation) {
    while (state_.seen_shops < observation.n_shops) {
        const int instance = state_.seen_shops++;
        const int shop = observation.shops[instance];
        if (shop < 0 || shop >= kag::N_SHOPS) std::abort();
        ++state_.instance_count[shop];
        state_.reveal_step[instance] = observation.step;
        for (int product = 0; product < kag::N_PRODUCTS; ++product)
            state_.demand[product] += SHOP_DEMAND[shop][product];
    }
    state_.cash = static_cast<int>(observation.self().money);
    state_.workers = observation.self().n_units;
    state_.shed_total = observation.own.shed_total;
    state_.remaining_steps = 719 - observation.step;
    state_.growing_crops.fill(0);
    state_.animals.fill(0);
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        state_.shed[item] = observation.own.shed[item];
        int cargo = 0;
        for (int unit = 0; unit < observation.self().n_units; ++unit)
            cargo += observation.own.inv[unit][item];
        state_.cargo[item] = static_cast<int16_t>(cargo);
    }
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        state_.seeds[crop] = observation.own.seeds[crop];
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            const kag::Tile& tile = observation.self().tiles[y][x];
            if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS)
                ++state_.growing_crops[tile.what];
            if (tile.has_animal && tile.what >= kag::GOOSE &&
                tile.what < kag::N_ITEMS)
                ++state_.animals[tile.what - kag::GOOSE];
        }
    for (int product = 0; product < kag::N_PRODUCTS; ++product) {
        state_.market_inventory[product] =
            observation.market.inventory[product];
        state_.market_price[product] = observation.market.prices[product];
    }
}

void Agent::apply_second_reveal_replan(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (state_.seen_shops < 2) return;
    const int headroom = observation.own.shed_total;
    for (int product = 0; product < kag::N_PRODUCTS; ++product) {
        const int per_demand = parameters_.reserve_per_demand[product];
        if (!per_demand || !state_.demand[product] ||
            observation.step >= parameters_.release_step[product])
            continue;
        int reserve = per_demand * state_.demand[product];
        if (headroom > 100 - parameters_.minimum_headroom)
            reserve = std::min(reserve,
                               std::max(0, 100 - headroom));
        int sellable = std::max<int>(0, observation.own.shed[product] - reserve);
        bool changed = false;
        for (int order = 0; order < action.n_orders; ++order) {
            kag::Order& current = action.orders[order];
            if (current.op != kag::M_SELL || current.item != product)
                continue;
            const int kept = std::min<int>(current.n, sellable);
            sellable -= kept;
            changed |= kept != current.n;
            current.n = kept;
        }
        if (changed) ++state_.repair_activations;
    }
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    action.finalize();
}

void Agent::repair_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!parameters_.capacity_repair || observation.n_shops < 2)
        return;
    const uint8_t first_bit = uint8_t{1} << observation.shops[0];
    const uint8_t second_bit = uint8_t{1} << observation.shops[1];
    const bool alternate_ice =
        observation.step >= parameters_.alternate_ice_start_step &&
        (((parameters_.alternate_ice_first_mask & first_bit) &&
         (parameters_.alternate_ice_second_mask & second_bit)) ||
        ((parameters_.alternate_ice_extra_first_mask & first_bit) &&
         (parameters_.alternate_ice_extra_second_mask & second_bit)));
    const bool alternate_brunch =
        (parameters_.alternate_brunch_first_mask & first_bit) &&
        (parameters_.alternate_brunch_second_mask & second_bit) &&
        observation.step >= parameters_.alternate_brunch_start_step &&
        observation.step < parameters_.alternate_brunch_end_step;
    const bool yarn_alternate_ice = alternate_ice &&
        observation.shops[0] == kag::SHOP_YARN_STORE;
    if (observation.hour != 23 &&
        !(yarn_alternate_ice && observation.hour == 22))
        return;
    if (observation.shops[0] != kag::SHOP_PIZZA_SHOP && !alternate_ice &&
        !alternate_brunch)
        return;
    std::array<int, kag::N_ITEMS> available{};
    int inventory = observation.own.shed_total;
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        available[item] = observation.own.shed[item];
        for (int unit = 0; unit < observation.self().n_units; ++unit)
            inventory += observation.own.inv[unit][item];
    }
    if ((yarn_alternate_ice || alternate_brunch) &&
        observation.hour == 23)
        for (int unit = 0; unit < observation.self().n_units; ++unit) {
            const kag::UnitAction& unit_action = action.units[unit];
            const kag::Tile& tile = observation.self().tiles
                [observation.self().pos_y[unit]]
                [observation.self().pos_x[unit]];
            if (unit_action.op == kag::OP_HARVEST && tile.yield_units > 0 &&
                ((tile.kind == kag::T_PLANT &&
                  observation.day - tile.planted_day >=
                      kag::CROPS[tile.what].first_yield_day) ||
                 tile.has_animal))
                inventory += tile.yield_units;
            if (unit_action.op == kag::OP_COLLECT_FERTILIZER &&
                tile.has_animal && tile.fertilizer_available)
                ++inventory;
        }
    for (int order = 0; order < action.n_orders; ++order) {
        const kag::Order& current = action.orders[order];
        if (current.op == kag::M_SELL && current.item < kag::N_ITEMS) {
            const int sold = std::min<int>(available[current.item], current.n);
            available[current.item] -= sold;
            inventory -= sold;
        } else if ((current.op == kag::M_BUY_PRODUCT ||
                    current.op == kag::M_BUY_ANIMAL) &&
                   current.item < kag::N_ITEMS) {
            inventory += current.n;
            available[current.item] += current.n;
        }
    }
    int overflow = yarn_alternate_ice && observation.hour == 22 ? 4 :
        inventory - 100;
    if (overflow <= 0) return;
    constexpr std::array<int, kag::N_PRODUCTS> sale_priority = {
        kag::FERTILIZER, kag::WHEAT, kag::EGG, kag::CARROT, kag::TOMATO,
        kag::STRAWBERRY, kag::MILK, kag::WOOL, kag::MELON};
    for (int item : sale_priority) {
        const int quantity = std::min(overflow, available[item]);
        if (!quantity) continue;
        bool merged = false;
        for (int order = 0; order < action.n_orders; ++order)
            if (action.orders[order].op == kag::M_SELL &&
                action.orders[order].item == item) {
                action.orders[order].n += quantity;
                merged = true;
                break;
            }
        if (!merged) {
            if (action.n_orders == 10) continue;
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item), quantity};
        }
        available[item] -= quantity;
        overflow -= quantity;
        if (!overflow) break;
    }
    if (overflow < inventory - 100) ++state_.repair_activations;
    action.finalize();
}

void Agent::repair_terminal_seeds(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!parameters_.terminal_seed_repair || observation.n_shops < 2)
        return;
    const uint8_t first_bit = uint8_t{1} << observation.shops[0];
    const uint8_t second_bit = uint8_t{1} << observation.shops[1];
    const bool original_repair =
        (observation.shops[0] == kag::SHOP_BRUNCH_SPOT ||
         observation.shops[0] == kag::SHOP_FARMERS_MARKET) &&
        observation.shops[1] != kag::SHOP_YARN_STORE;
    const bool alternate_repair = parameters_.alternate_ice_seed_repair &&
        observation.step >= parameters_.alternate_ice_start_step &&
        (((parameters_.alternate_ice_first_mask & first_bit) &&
          (parameters_.alternate_ice_second_mask & second_bit)) ||
         ((parameters_.alternate_ice_extra_first_mask & first_bit) &&
          (parameters_.alternate_ice_extra_second_mask & second_bit)));
    if (!original_repair && !alternate_repair) return;
    int crop = -1;
    int reduction = 0;
    if (alternate_repair &&
        observation.shops[0] == kag::SHOP_YARN_STORE &&
        observation.step == 528) {
        crop = kag::WHEAT;
        reduction = 5;
    } else if (observation.step == 360) {
        crop = kag::TOMATO;
        reduction = 1;
    } else if (observation.step == 552) {
        crop = kag::WHEAT;
        reduction = 5;
    } else if (observation.step == 576) {
        crop = kag::WHEAT;
        reduction = 7;
    } else if (observation.step == 600) {
        crop = kag::WHEAT;
        reduction = 2;
    } else if (observation.step == 624) {
        crop = kag::CARROT;
        reduction = 5;
    }
    if (crop < 0) return;
    bool changed = false;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op != kag::M_BUY_SEED || current.item != crop) continue;
        const int removed = std::min(current.n, reduction);
        current.n -= removed;
        reduction -= removed;
        changed |= removed > 0;
    }
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    if (changed) ++state_.repair_activations;
    action.finalize();
}

void Agent::repair_terminal_eggs(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if ((observation.step != 716 && observation.step != 717) ||
        observation.n_shops < 2 ||
        (observation.shops[0] != kag::SHOP_BRUNCH_SPOT &&
         observation.shops[0] != kag::SHOP_FARMERS_MARKET) ||
        (observation.shops[1] != kag::SHOP_BAKERY &&
         observation.shops[1] != kag::SHOP_PET_CAFE))
        return;
    if (observation.step == 716) {
        std::array<int, kag::N_ITEMS> available{};
        for (int item = 0; item < kag::N_ITEMS; ++item)
            available[item] = observation.own.shed[item];
        for (int order = 0; order < action.n_orders; ++order) {
            const kag::Order& market = action.orders[order];
            if (market.op == kag::M_SELL && market.item < kag::N_ITEMS)
                available[market.item] -=
                    std::min<int>(available[market.item], market.n);
        }
        constexpr std::array<int, kag::N_PRODUCTS> priority = {
            kag::FERTILIZER, kag::WHEAT, kag::EGG, kag::CARROT, kag::TOMATO,
            kag::STRAWBERRY, kag::MILK, kag::WOOL, kag::MELON};
        int headroom = parameters_.terminal_egg_headroom;
        const int initial_headroom = headroom;
        for (int item : priority) {
            const int quantity = std::min(headroom, available[item]);
            if (!quantity) continue;
            bool merged = false;
            for (int order = 0; order < action.n_orders; ++order)
                if (action.orders[order].op == kag::M_SELL &&
                    action.orders[order].item == item) {
                    action.orders[order].n += quantity;
                    merged = true;
                    break;
                }
            if (!merged && action.n_orders < 10)
                action.orders[action.n_orders++] = {
                    kag::M_SELL, static_cast<uint8_t>(item), quantity};
            else if (!merged)
                continue;
            available[item] -= quantity;
            headroom -= quantity;
            if (!headroom) break;
        }
        if (headroom != initial_headroom) {
            ++state_.repair_activations;
            action.finalize();
        }
        return;
    }
    if (observation.step == 717)
        for (int unit = 0; unit < observation.self().n_units; ++unit) {
            const int eggs = observation.own.inv[unit][kag::EGG];
            if (!eggs || action.units[unit].op != kag::OP_PASS) continue;
            for (int order = 0; order < action.n_orders; ++order) {
                kag::Order& market = action.orders[order];
                if (market.op != kag::M_SELL || market.item != kag::EGG)
                    continue;
                action.units[unit] = {kag::OP_PLACE, kag::EGG, eggs};
                market.n += eggs;
                ++state_.repair_activations;
                action.finalize();
                return;
            }
        }
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    if (!initialized_) std::abort();
    update_state(observation);
    warm_.act(observation, budget, action);
    if (parameters_.fallback_first_mask &&
        parameters_.fallback_second_mask) {
        kag::Action fallback_action;
        fallback_warm_->act(observation, budget, fallback_action);
        const bool use_post_fallback = observation.n_shops >= 2 &&
            observation.step >= parameters_.fallback_start_step &&
            (observation.step < parameters_.fallback_end_step ||
             (parameters_.fallback_persistent_mask &
              (uint8_t{1} << observation.shops[1]))) &&
            (parameters_.fallback_first_mask &
             (uint8_t{1} << observation.shops[0])) &&
            (parameters_.fallback_second_mask &
             (uint8_t{1} << observation.shops[1]));
        if (use_post_fallback)
            action = fallback_action;
    }
    if ((parameters_.alternate_ice_first_mask &&
         parameters_.alternate_ice_second_mask) ||
        (parameters_.alternate_ice_extra_first_mask &&
         parameters_.alternate_ice_extra_second_mask)) {
        kag::Action alternate_action;
        alternate_ice_warm_->act(observation, budget, alternate_action);
        if (observation.n_shops >= 2 &&
            observation.step >= parameters_.alternate_ice_start_step) {
            const uint8_t first_bit = uint8_t{1} << observation.shops[0];
            const uint8_t second_bit = uint8_t{1} << observation.shops[1];
            const bool use_alternate =
                ((parameters_.alternate_ice_first_mask & first_bit) &&
                 (parameters_.alternate_ice_second_mask & second_bit)) ||
                ((parameters_.alternate_ice_extra_first_mask & first_bit) &&
                 (parameters_.alternate_ice_extra_second_mask & second_bit));
            if (use_alternate) action = alternate_action;
        }
    }
    if (parameters_.alternate_yarn_first_mask &&
        parameters_.alternate_yarn_second_mask) {
        kag::Action alternate_action;
        alternate_yarn_warm_->act(observation, budget, alternate_action);
        if (observation.n_shops >= 2 &&
            observation.step >= parameters_.alternate_yarn_start_step &&
            (parameters_.alternate_yarn_first_mask &
             (uint8_t{1} << observation.shops[0])) &&
            (parameters_.alternate_yarn_second_mask &
             (uint8_t{1} << observation.shops[1])))
            action = alternate_action;
    }
    if (parameters_.alternate_pizza_first_mask &&
        parameters_.alternate_pizza_second_mask) {
        kag::Action alternate_action;
        alternate_pizza_warm_->act(observation, budget, alternate_action);
        if (observation.n_shops >= 2 &&
            observation.step >= parameters_.alternate_pizza_start_step &&
            (parameters_.alternate_pizza_first_mask &
             (uint8_t{1} << observation.shops[0])) &&
            (parameters_.alternate_pizza_second_mask &
             (uint8_t{1} << observation.shops[1])))
            action = alternate_action;
    }
    if (parameters_.alternate_brunch_first_mask &&
        parameters_.alternate_brunch_second_mask) {
        kag::Action alternate_action;
        alternate_brunch_warm_->act(observation, budget, alternate_action);
        if (observation.n_shops >= 2 &&
            observation.step >= parameters_.alternate_brunch_start_step &&
            observation.step < parameters_.alternate_brunch_end_step &&
            (parameters_.alternate_brunch_first_mask &
             (uint8_t{1} << observation.shops[0])) &&
            (parameters_.alternate_brunch_second_mask &
             (uint8_t{1} << observation.shops[1])))
            action = alternate_action;
    }
    repair_terminal_seeds(observation, action);
    apply_second_reveal_replan(observation, action);
    repair_capacity(observation, action);
    repair_terminal_eggs(observation, action);
}

}  // namespace kag::agents::four_random_shop::base
