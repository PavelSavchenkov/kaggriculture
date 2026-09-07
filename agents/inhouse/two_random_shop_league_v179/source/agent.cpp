#include "agents/inhouse/two_random_shop_league_v179/source/agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace kag::agents::two_random_shop_league_v179 {
namespace {

void configure_adversarial(base::ReplanParameters& parameters) {
    parameters.branch_delegate[kag::SHOP_PIZZA_SHOP] =
        kag::SHOP_ICE_CREAM_SHOP;
    parameters.alternate_pizza_first_mask =
        uint8_t{1} << kag::SHOP_PIZZA_SHOP;
    parameters.alternate_pizza_start_step = 216;
}

bool reduce_order(kag::Action& action, int op, int item, int reduction) {
    bool changed = false;
    for (int order = 0; order < action.n_orders && reduction; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op != op || current.item != item) continue;
        const int removed = std::min(current.n, reduction);
        current.n -= removed;
        reduction -= removed;
        changed |= removed > 0;
    }
    if (!changed) return false;
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    action.finalize();
    return true;
}

}  // namespace

kag::agent::AgentInfo Agent::info() {
    return {"two_random_shop_league_v179"};
}

void Agent::configure() {
    if (configured_ || initialized_) std::abort();

    base::ReplanParameters primary;
    configure_adversarial(primary);
    primary.terminal_egg_headroom = 14;
    c0_.set_parameters(primary);

    base::ReplanParameters pet;
    configure_adversarial(pet);
    pet.alternate_ice_first_mask = 0;
    pet.alternate_ice_second_mask = 0;
    pet.alternate_ice_extra_first_mask = 0;
    pet.alternate_ice_extra_second_mask = 0;
    pet.branch_delegate[kag::SHOP_YARN_STORE] = kag::SHOP_PET_CAFE;
    alternate_pet_.set_parameters(pet);

    base::ReplanParameters target;
    configure_adversarial(target);
    target.alternate_ice_first_mask = 0;
    target.alternate_ice_second_mask = 0;
    target.alternate_ice_extra_first_mask = 0;
    target.alternate_ice_extra_second_mask = 0;
    target.branch_delegate[kag::SHOP_YARN_STORE] =
        kag::SHOP_ICE_CREAM_SHOP;
    target.terminal_egg_headroom = 14;
    alternate_target_.set_parameters(target);

    configured_ = true;
}

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!configured_) configure();
    c0_.reset(init);
    alternate_pet_.reset(init);
    alternate_target_.reset(init);
    held_units_ = 0;
    initialized_ = true;
}

void Agent::apply_market(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (observation.step > 143 || !observation.n_shops ||
        observation.shops[0] != kag::SHOP_YARN_STORE)
        return;
    if (observation.step == 119) {
        if (action.n_orders >= 10) return;
        const int price = std::max(
            1, observation.market.prices[kag::WHEAT]);
        const int quantity = std::min({
            5, static_cast<int>(observation.self().money) / (price + 16),
            100 - observation.own.shed_total});
        if (quantity <= 0) return;
        for (int order = action.n_orders - 1; order >= 0; --order)
            action.orders[order + 1] = action.orders[order];
        action.orders[0] = {
            kag::M_BUY_PRODUCT, kag::WHEAT, quantity};
        ++action.n_orders;
        held_units_ = quantity;
        action.finalize();
        return;
    }
    if (observation.step != 122) return;
    const int quantity = std::min<int>(
        held_units_, observation.own.shed[kag::WHEAT]);
    if (quantity <= 0) return;
    bool merged = false;
    for (int order = 0; order < action.n_orders; ++order)
        if (action.orders[order].op == kag::M_SELL &&
            action.orders[order].item == kag::WHEAT) {
            action.orders[order].n += quantity;
            merged = true;
            break;
        }
    if (!merged) {
        if (action.n_orders >= 10) return;
        action.orders[action.n_orders++] = {
            kag::M_SELL, kag::WHEAT, quantity};
    }
    held_units_ = 0;
    action.finalize();
}

void Agent::repair_transition_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_YARN_STORE &&
        observation.shops[1] == kag::SHOP_SMOOTHIE_SHOP) {
        int crop = -1;
        int reduction = 0;
        if (observation.step == 528 &&
            observation.own.seeds[kag::WHEAT] >= 12) {
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
        reduce_order(action, kag::M_BUY_SEED, crop, reduction);
    }

    if (observation.n_shops >= 2 &&
        (observation.shops[0] == kag::SHOP_BAKERY ||
         observation.shops[0] == kag::SHOP_PET_CAFE) &&
        (observation.shops[1] == kag::SHOP_BRUNCH_SPOT ||
         observation.shops[1] == kag::SHOP_FARMERS_MARKET)) {
        int crop = -1;
        int reduction = 0;
        if (observation.step == 552) {
            crop = kag::WHEAT;
            reduction = 3;
        } else if (observation.step == 576) {
            crop = kag::WHEAT;
            reduction = 7;
        } else if (observation.step == 600) {
            crop = kag::WHEAT;
            reduction = 2;
        } else if (observation.step == 624 || observation.step == 648) {
            crop = kag::CARROT;
            reduction = 2;
        }
        reduce_order(action, kag::M_BUY_SEED, crop, reduction);
    }

    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
        (observation.shops[1] == kag::SHOP_BAKERY ||
         observation.shops[1] == kag::SHOP_PET_CAFE)) {
        int crop = -1;
        int reduction = 0;
        if (observation.step == 360) {
            crop = kag::TOMATO;
            reduction = 1;
        } else if (observation.step == 552) {
            crop = kag::WHEAT;
            reduction = 6;
        } else if (observation.step == 576) {
            crop = kag::WHEAT;
            reduction = 7;
        } else if (observation.step == 600) {
            crop = kag::WHEAT;
            reduction = 2;
        } else if (observation.step == 624) {
            crop = kag::CARROT;
            reduction = 3;
        } else if (observation.step == 648) {
            crop = kag::CARROT;
            reduction = 2;
        }
        reduce_order(action, kag::M_BUY_SEED, crop, reduction);
    }

    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_ICE_CREAM_SHOP &&
        observation.step == 576)
        reduce_order(action, kag::M_BUY_SEED, kag::WHEAT, 1);

    const uint8_t late_wheat_second_mask =
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET) |
        (uint8_t{1} << kag::SHOP_ICE_CREAM_SHOP) |
        (uint8_t{1} << kag::SHOP_PIZZA_SHOP) |
        (uint8_t{1} << kag::SHOP_SMOOTHIE_SHOP);
    if (observation.n_shops >= 2 &&
        (late_wheat_second_mask &
         (uint8_t{1} << observation.shops[1])) &&
        ((observation.shops[0] == kag::SHOP_FARMERS_MARKET &&
          observation.step == 552) ||
         (observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
          observation.step == 600)))
        reduce_order(action, kag::M_BUY_SEED, kag::WHEAT, 1);

    const uint8_t pizza_extra_wheat_second_mask =
        (uint8_t{1} << kag::SHOP_BRUNCH_SPOT) |
        (uint8_t{1} << kag::SHOP_FARMERS_MARKET) |
        (uint8_t{1} << kag::SHOP_ICE_CREAM_SHOP) |
        (uint8_t{1} << kag::SHOP_SMOOTHIE_SHOP);
    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
        (pizza_extra_wheat_second_mask &
         (uint8_t{1} << observation.shops[1])) &&
        observation.step == 600)
        reduce_order(action, kag::M_BUY_SEED, kag::WHEAT, 1);
    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
        (pizza_extra_wheat_second_mask &
         (uint8_t{1} << observation.shops[1])) &&
        observation.step == 552 &&
        observation.own.seeds[kag::WHEAT] >= 10)
        reduce_order(action, kag::M_BUY_SEED, kag::WHEAT, 1);

    const bool primary_strawberry_pair = observation.n_shops >= 2 &&
        ((observation.shops[0] == kag::SHOP_BRUNCH_SPOT &&
          observation.shops[1] == kag::SHOP_FARMERS_MARKET) ||
         (observation.shops[0] == kag::SHOP_SMOOTHIE_SHOP &&
          (observation.shops[1] == kag::SHOP_SMOOTHIE_SHOP ||
           observation.shops[1] == kag::SHOP_YARN_STORE)) ||
         (observation.shops[0] == kag::SHOP_YARN_STORE &&
          observation.shops[1] == kag::SHOP_SMOOTHIE_SHOP));
    const bool additional_strawberry_pair = observation.n_shops >= 2 &&
        ((observation.shops[0] == kag::SHOP_YARN_STORE &&
          observation.shops[1] == kag::SHOP_ICE_CREAM_SHOP) ||
         (observation.shops[0] == kag::SHOP_BAKERY &&
          observation.shops[1] == kag::SHOP_ICE_CREAM_SHOP) ||
         (observation.shops[0] == kag::SHOP_PET_CAFE &&
          observation.shops[1] == kag::SHOP_SMOOTHIE_SHOP) ||
         (observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
          observation.shops[1] == kag::SHOP_BAKERY) ||
         (observation.shops[0] == kag::SHOP_SMOOTHIE_SHOP &&
          observation.shops[1] == kag::SHOP_PET_CAFE));
    const bool strawberry_pair =
        primary_strawberry_pair || additional_strawberry_pair;
    if (strawberry_pair && observation.step == 336 &&
        observation.own.seeds[kag::STRAWBERRY] >= 5)
        reduce_order(action, kag::M_BUY_SEED, kag::STRAWBERRY, 2);
    if (strawberry_pair && observation.step == 360 &&
        observation.own.seeds[kag::STRAWBERRY] >= 3)
        reduce_order(action, kag::M_BUY_SEED, kag::STRAWBERRY, 1);

    if (observation.n_shops >= 2 &&
        observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
        observation.shops[1] == kag::SHOP_PIZZA_SHOP &&
        observation.step == 408 && observation.own.shed[kag::SHEEP] >= 1)
        reduce_order(action, kag::M_BUY_ANIMAL, kag::SHEEP, 1);

    if (observation.step == 695 && observation.n_shops >= 2) {
        int carried = 0;
        for (int unit = 0; unit < observation.self().n_units; ++unit)
            for (int item = 0; item < kag::N_ITEMS; ++item)
                carried += observation.own.inv[unit][item];
        const int overflow = std::max(0, carried - 100);
        int selected = -1;
        int selected_total = 1000000;
        for (int unit = 0; unit < observation.self().n_units; ++unit) {
            if (action.units[unit].op != kag::OP_PASS ||
                !kag::is_shed_adjacent(observation.self().pos_x[unit],
                                       observation.self().pos_y[unit],
                                       kag::BOARD))
                continue;
            int total = 0;
            int new_orders = 0;
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                if (!observation.own.inv[unit][item]) continue;
                total += observation.own.inv[unit][item];
                bool merged = false;
                for (int order = 0; order < action.n_orders; ++order)
                    merged |= action.orders[order].op == kag::M_SELL &&
                        action.orders[order].item == item;
                new_orders += !merged;
            }
            if (total >= overflow && total < selected_total &&
                action.n_orders + new_orders <= 10) {
                selected = unit;
                selected_total = total;
            }
        }
        if (overflow && selected >= 0) {
            action.units[selected] = {kag::OP_DROP, 0, 1};
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                const int quantity = observation.own.inv[selected][item];
                if (!quantity) continue;
                bool merged = false;
                for (int order = 0; order < action.n_orders; ++order)
                    if (action.orders[order].op == kag::M_SELL &&
                        action.orders[order].item == item) {
                        action.orders[order].n += quantity;
                        merged = true;
                        break;
                    }
                if (!merged)
                    action.orders[action.n_orders++] = {
                        kag::M_SELL, static_cast<uint8_t>(item), quantity};
            }
            action.finalize();
        }
    }

    if (observation.step != 264 || observation.n_shops < 2 ||
        observation.shops[0] != kag::SHOP_YARN_STORE ||
        observation.shops[1] != kag::SHOP_FARMERS_MARKET)
        return;
    std::array<int, kag::N_ITEMS> available{};
    for (int item = 0; item < kag::N_ITEMS; ++item)
        available[item] = observation.own.shed[item];
    int inventory = observation.own.shed_total;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op == kag::M_SELL && current.item < kag::N_ITEMS) {
            const int quantity = std::min<int>(
                current.n, available[current.item]);
            available[current.item] -= quantity;
            inventory -= quantity;
        } else if ((current.op == kag::M_BUY_PRODUCT ||
                    current.op == kag::M_BUY_ANIMAL) &&
                   current.item < kag::N_ITEMS) {
            current.n = std::min<int>(current.n,
                                      std::max(0, 100 - inventory));
            inventory += current.n;
            available[current.item] += current.n;
        }
    }
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    action.finalize();
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    if (!initialized_) std::abort();
    c0_.act(observation, budget, action);

    kag::Action pet_action;
    alternate_pet_.act(observation, budget, pet_action);
    kag::Action target_action;
    alternate_target_.act(observation, budget, target_action);

    const bool use_between_reveals = observation.n_shops == 1 &&
        observation.step >= 96 &&
        observation.shops[0] == kag::SHOP_YARN_STORE;
    const bool use_after_second = observation.n_shops >= 2 &&
        observation.step >= 144 &&
        observation.shops[0] == kag::SHOP_YARN_STORE &&
        observation.shops[1] == kag::SHOP_SMOOTHIE_SHOP;
    if (use_between_reveals) action = pet_action;
    if (use_after_second) action = target_action;

    apply_market(observation, action);
    repair_transition_capacity(observation, action);
}

}  // namespace kag::agents::two_random_shop_league_v179
