#include "agents/inhouse/four_random_shop/source/agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace kag::agents::four_random_shop {

kag::agent::AgentInfo Agent::info() { return {"four_random_shop"}; }

void Agent::set_parameters(const Parameters& parameters) {
    if (initialized_ || configured_ || parameters.milk_reserve_per_demand > 100 ||
        parameters.milk_release_step > 719 ||
        (parameters.forced_mode != 255 &&
         parameters.forced_mode >= kag::N_SHOPS) ||
        parameters.forced_mode_start_step > 720 || parameters.mode_rule > 31 ||
        parameters.third_yarn_rule > 2 || parameters.third_pizza_rule > 5 ||
        parameters.fourth_pizza_rule > 16 || parameters.fourth_yarn_rule > 1 ||
        parameters.third_yarn_start_step < 216 ||
        parameters.third_yarn_start_step > 719 ||
        parameters.third_pizza_start_step < 216 ||
        parameters.third_pizza_start_step > 719 ||
        parameters.minimum_headroom > 100 ||
        !parameters.terminal_egg_headroom ||
        parameters.terminal_egg_headroom > 20 ||
        parameters.pizza_step695_second_milk > 50 ||
        parameters.smoothie_rule > 1 ||
        parameters.opening_fertilizer_sale > 20 ||
        (parameters.pet_day11_melon_sale != 255 &&
         parameters.pet_day11_melon_sale > 50) ||
        parameters.pet_day11_wheat_shift > 16)
        std::abort();
    parameters_ = parameters;
}

void Agent::configure() {
    if (configured_) return;
    base::ReplanParameters base_parameters;
    base_parameters.reserve_per_demand[kag::MILK] =
        parameters_.milk_reserve_per_demand;
    base_parameters.release_step[kag::MILK] = parameters_.milk_release_step;
    base_parameters.minimum_headroom = parameters_.minimum_headroom;
    base_parameters.terminal_egg_headroom =
        parameters_.terminal_egg_headroom;
    base_parameters.pizza_step695_second_milk =
        parameters_.pizza_step695_second_milk;
    base_parameters.opening_fertilizer_sale =
        parameters_.opening_fertilizer_sale;
    base_parameters.pet_reveal_wheat = parameters_.pet_reveal_wheat;
    base_parameters.flexible_animal = parameters_.flexible_animal;
    base_.set_parameters(base_parameters);
    const auto configure_mode = [&](base::Agent& agent, uint8_t shop) {
        base::ReplanParameters mode_parameters = base_parameters;
        mode_parameters.branch_delegate.fill(shop);
        mode_parameters.fallback_first_mask = 0;
        mode_parameters.fallback_second_mask = 0;
        mode_parameters.fallback_persistent_mask = 0;
        mode_parameters.alternate_ice_first_mask = 0;
        mode_parameters.alternate_ice_second_mask = 0;
        mode_parameters.alternate_ice_extra_first_mask = 0;
        mode_parameters.alternate_ice_extra_second_mask = 0;
        mode_parameters.alternate_yarn_first_mask = 0;
        mode_parameters.alternate_yarn_second_mask = 0;
        mode_parameters.alternate_pizza_first_mask = 0;
        mode_parameters.alternate_pizza_second_mask = 0;
        mode_parameters.alternate_brunch_first_mask = 0;
        mode_parameters.alternate_brunch_second_mask = 0;
        agent.set_parameters(mode_parameters);
    };
    if (parameters_.forced_mode != 255)
        configure_mode(mode_, parameters_.forced_mode);
    if (parameters_.third_yarn_rule || parameters_.fourth_yarn_rule)
        configure_mode(yarn_, kag::SHOP_YARN_STORE);
    if (parameters_.third_pizza_rule || parameters_.fourth_pizza_rule)
        configure_mode(pizza_, kag::SHOP_PIZZA_SHOP);
    if (parameters_.smoothie_rule) {
        smoothie_ = std::make_unique<base::Agent>();
        configure_mode(*smoothie_, kag::SHOP_SMOOTHIE_SHOP);
    }
    configured_ = true;
}

bool Agent::use_yarn(
    const kag::agent::AgentObservation& observation) const {
    if (observation.step < parameters_.third_yarn_start_step ||
        observation.n_shops < 3)
        return false;
    const int first = observation.shops[0];
    const int second = observation.shops[1];
    const int third = observation.shops[2];
    if (parameters_.fourth_yarn_rule && observation.step >= 288 &&
        observation.n_shops >= 4 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        second == kag::SHOP_YARN_STORE &&
        (third == kag::SHOP_ICE_CREAM_SHOP ||
         third == kag::SHOP_SMOOTHIE_SHOP) &&
        (observation.shops[3] == kag::SHOP_BAKERY ||
         observation.shops[3] == kag::SHOP_PET_CAFE ||
         observation.shops[3] == kag::SHOP_YARN_STORE))
        return true;
    if (!parameters_.third_yarn_rule || third != kag::SHOP_YARN_STORE)
        return false;
    const bool core =
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET ||
         second == kag::SHOP_PET_CAFE || second == kag::SHOP_PIZZA_SHOP);
    if (core || parameters_.third_yarn_rule == 1) return core;
    const bool tail =
        (first == kag::SHOP_BRUNCH_SPOT ||
         first == kag::SHOP_FARMERS_MARKET ||
         first == kag::SHOP_PIZZA_SHOP) &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE);
    return core || tail;
}

bool Agent::use_pizza(
    const kag::agent::AgentObservation& observation) const {
    if (observation.step < parameters_.third_pizza_start_step ||
        observation.n_shops < 3)
        return false;
    const int first = observation.shops[0];
    const int second = observation.shops[1];
    const int third = observation.shops[2];
    if (parameters_.fourth_pizza_rule >= 6 && observation.step >= 288 &&
        observation.n_shops >= 4 && first == kag::SHOP_PIZZA_SHOP &&
        (third == kag::SHOP_ICE_CREAM_SHOP ||
         third == kag::SHOP_SMOOTHIE_SHOP)) {
        const int fourth = observation.shops[3];
        const bool fourth_bakery_pet_pizza =
            fourth == kag::SHOP_BAKERY || fourth == kag::SHOP_PET_CAFE ||
            fourth == kag::SHOP_PIZZA_SHOP;
        if ((second == kag::SHOP_PIZZA_SHOP &&
             fourth == kag::SHOP_PIZZA_SHOP) ||
            (parameters_.fourth_pizza_rule >= 7 &&
             second == kag::SHOP_PIZZA_SHOP &&
             fourth_bakery_pet_pizza) ||
            (parameters_.fourth_pizza_rule >= 8 &&
             second == kag::SHOP_PIZZA_SHOP &&
             fourth == kag::SHOP_YARN_STORE))
            return true;
    }
    if (parameters_.fourth_pizza_rule >= 9 && observation.step >= 288 &&
        observation.n_shops >= 4) {
        const int fourth = observation.shops[3];
        if (first == kag::SHOP_BAKERY && second == kag::SHOP_BAKERY &&
            third == kag::SHOP_BAKERY && fourth == kag::SHOP_PIZZA_SHOP)
            return true;
        if (parameters_.fourth_pizza_rule >= 10 &&
            first == kag::SHOP_BAKERY && second == kag::SHOP_BAKERY &&
            third == kag::SHOP_YARN_STORE &&
            (fourth == kag::SHOP_ICE_CREAM_SHOP ||
             fourth == kag::SHOP_SMOOTHIE_SHOP))
            return true;
        if (parameters_.fourth_pizza_rule >= 11 &&
            first == kag::SHOP_BAKERY && second == kag::SHOP_PIZZA_SHOP &&
            third == kag::SHOP_YARN_STORE &&
            (fourth == kag::SHOP_BAKERY ||
             fourth == kag::SHOP_BRUNCH_SPOT ||
             fourth == kag::SHOP_FARMERS_MARKET ||
             fourth == kag::SHOP_PET_CAFE ||
             fourth == kag::SHOP_PIZZA_SHOP ||
             fourth == kag::SHOP_YARN_STORE))
            return true;
        if (parameters_.fourth_pizza_rule >= 12 &&
            first == kag::SHOP_BAKERY && second == kag::SHOP_YARN_STORE &&
            third == kag::SHOP_PIZZA_SHOP &&
            (fourth == kag::SHOP_BAKERY || fourth == kag::SHOP_PET_CAFE ||
             fourth == kag::SHOP_PIZZA_SHOP ||
             fourth == kag::SHOP_YARN_STORE))
            return true;
        if (parameters_.fourth_pizza_rule >= 13 &&
            first == kag::SHOP_BAKERY && second == kag::SHOP_YARN_STORE &&
            (third == kag::SHOP_BAKERY || third == kag::SHOP_YARN_STORE) &&
            fourth == kag::SHOP_PIZZA_SHOP)
            return true;
        if (parameters_.fourth_pizza_rule >= 14 &&
            first == kag::SHOP_PET_CAFE && second == kag::SHOP_PIZZA_SHOP &&
            third == kag::SHOP_YARN_STORE &&
            (fourth == kag::SHOP_BAKERY || fourth == kag::SHOP_PET_CAFE ||
             fourth == kag::SHOP_PIZZA_SHOP ||
             fourth == kag::SHOP_YARN_STORE))
            return true;
        if (parameters_.fourth_pizza_rule >= 15 &&
            first == kag::SHOP_PET_CAFE && second == kag::SHOP_YARN_STORE &&
            third == kag::SHOP_PIZZA_SHOP &&
            fourth == kag::SHOP_PIZZA_SHOP)
            return true;
        if (parameters_.fourth_pizza_rule >= 16 &&
            first == kag::SHOP_PIZZA_SHOP && second == kag::SHOP_PET_CAFE &&
            third == kag::SHOP_YARN_STORE)
            return true;
    }
    if (parameters_.fourth_pizza_rule == 1 && observation.step >= 288 &&
        observation.n_shops >= 4 && first == kag::SHOP_PIZZA_SHOP &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
        (third == kag::SHOP_ICE_CREAM_SHOP ||
         third == kag::SHOP_SMOOTHIE_SHOP) &&
        (observation.shops[3] == kag::SHOP_BAKERY ||
         observation.shops[3] == kag::SHOP_PET_CAFE ||
         observation.shops[3] == kag::SHOP_PIZZA_SHOP))
        return true;
    if (parameters_.fourth_pizza_rule >= 2 && observation.step >= 288 &&
        observation.n_shops >= 4 && first == kag::SHOP_PIZZA_SHOP &&
        second == kag::SHOP_YARN_STORE &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
         third == kag::SHOP_YARN_STORE))
        return true;
    if (parameters_.fourth_pizza_rule >= 3 && observation.step >= 288 &&
        observation.n_shops >= 4 && first == kag::SHOP_PET_CAFE &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE) &&
        observation.shops[3] == kag::SHOP_PIZZA_SHOP)
        return true;
    if (parameters_.fourth_pizza_rule >= 4 && observation.step >= 288 &&
        observation.n_shops >= 4 && first == kag::SHOP_YARN_STORE &&
        second == kag::SHOP_PIZZA_SHOP &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
         third == kag::SHOP_YARN_STORE) &&
        observation.shops[3] == kag::SHOP_PIZZA_SHOP)
        return true;
    if (parameters_.fourth_pizza_rule >= 5 && observation.step >= 288 &&
        observation.n_shops >= 4 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE ||
         second == kag::SHOP_PIZZA_SHOP ||
         second == kag::SHOP_YARN_STORE) &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
         third == kag::SHOP_PIZZA_SHOP || third == kag::SHOP_YARN_STORE) &&
        !(first == kag::SHOP_BAKERY && second == kag::SHOP_BAKERY &&
          third == kag::SHOP_YARN_STORE) &&
        (observation.shops[3] == kag::SHOP_ICE_CREAM_SHOP ||
         observation.shops[3] == kag::SHOP_SMOOTHIE_SHOP))
        return true;
    if (!parameters_.third_pizza_rule) return false;
    const bool milk = third == kag::SHOP_ICE_CREAM_SHOP ||
        third == kag::SHOP_SMOOTHIE_SHOP;
    const bool narrow =
        (third == kag::SHOP_PIZZA_SHOP &&
         ((first == kag::SHOP_PIZZA_SHOP &&
           second == kag::SHOP_YARN_STORE) ||
          (first == kag::SHOP_YARN_STORE &&
           second == kag::SHOP_PIZZA_SHOP))) ||
        (first == kag::SHOP_BAKERY && second == kag::SHOP_PIZZA_SHOP &&
         (milk || third == kag::SHOP_PIZZA_SHOP));
    if (narrow || parameters_.third_pizza_rule == 1) return narrow;
    const bool medium =
        (first == kag::SHOP_BAKERY &&
         ((second == kag::SHOP_PET_CAFE &&
           (milk || third == kag::SHOP_PIZZA_SHOP)) ||
          (second == kag::SHOP_BAKERY &&
           (milk || third == kag::SHOP_PIZZA_SHOP)) ||
          ((second == kag::SHOP_BRUNCH_SPOT ||
            second == kag::SHOP_FARMERS_MARKET) &&
           third == kag::SHOP_PIZZA_SHOP))) ||
        (first == kag::SHOP_PET_CAFE &&
         (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
         third == kag::SHOP_PIZZA_SHOP);
    if (medium || parameters_.third_pizza_rule == 2)
        return narrow || medium;
    const bool wide = first == kag::SHOP_BAKERY && milk &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET);
    if (wide || parameters_.third_pizza_rule == 3)
        return narrow || medium || wide;
    const bool final =
        (first == kag::SHOP_PET_CAFE && second == kag::SHOP_PIZZA_SHOP &&
         (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE)) ||
        (first == kag::SHOP_BAKERY && second == kag::SHOP_PIZZA_SHOP &&
         (third == kag::SHOP_BAKERY || third == kag::SHOP_BRUNCH_SPOT ||
          third == kag::SHOP_FARMERS_MARKET ||
          third == kag::SHOP_PET_CAFE));
    if (parameters_.third_pizza_rule == 4)
        return narrow || medium || wide || final;
    return narrow || medium || final;
}

bool Agent::use_smoothie(
    const kag::agent::AgentObservation& observation) const {
    return parameters_.smoothie_rule && observation.step >= 288 &&
        observation.n_shops >= 4 &&
        observation.shops[0] == kag::SHOP_ICE_CREAM_SHOP &&
        (observation.shops[2] == kag::SHOP_ICE_CREAM_SHOP ||
         observation.shops[2] == kag::SHOP_SMOOTHIE_SHOP);
}

bool Agent::use_mode(
    const kag::agent::AgentObservation& observation) const {
    if (parameters_.forced_mode == 255 ||
        observation.step < parameters_.forced_mode_start_step)
        return false;
    if (parameters_.mode_rule == 0) return true;
    if (parameters_.mode_rule >= 25 && observation.n_shops) {
        const int early_first = observation.shops[0];
        if ((parameters_.mode_rule == 25 &&
             early_first == kag::SHOP_BAKERY) ||
            (parameters_.mode_rule == 26 &&
             early_first == kag::SHOP_PET_CAFE))
            return true;
    }
    if (observation.n_shops < 3) return false;
    const int first = observation.shops[0];
    const int second = observation.shops[1];
    const int third = observation.shops[2];
    const bool third_milk = third == kag::SHOP_ICE_CREAM_SHOP ||
        third == kag::SHOP_SMOOTHIE_SHOP;
    if (parameters_.mode_rule == 1)
        return first == kag::SHOP_PET_CAFE && third_milk;
    const bool unsafe_bakery = first == kag::SHOP_BAKERY &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET ||
         second == kag::SHOP_PET_CAFE || second == kag::SHOP_PIZZA_SHOP);
    const bool unsafe_brunch = first == kag::SHOP_BRUNCH_SPOT &&
        second == kag::SHOP_BAKERY;
    const bool unsafe_yarn = first == kag::SHOP_YARN_STORE &&
        (second == kag::SHOP_ICE_CREAM_SHOP ||
         second == kag::SHOP_SMOOTHIE_SHOP);
    const bool third_rule =
        third_milk && !unsafe_bakery && !unsafe_brunch && !unsafe_yarn;
    const bool third_tail_cluster =
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET ||
         (first == kag::SHOP_PET_CAFE && third == kag::SHOP_PIZZA_SHOP));
    const bool delayed_third_tail = parameters_.mode_rule >= 20 &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET);
    if (parameters_.mode_rule >= 5 && third_tail_cluster &&
        (!delayed_third_tail || observation.step >= 288))
        return true;
    const bool third_pizza_cluster = third == kag::SHOP_PIZZA_SHOP &&
        ((first == kag::SHOP_FARMERS_MARKET &&
          (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE ||
           second == kag::SHOP_YARN_STORE)) ||
         (first == kag::SHOP_BRUNCH_SPOT &&
          (second == kag::SHOP_PET_CAFE ||
           second == kag::SHOP_YARN_STORE)) ||
         (first == kag::SHOP_YARN_STORE &&
          (second == kag::SHOP_BRUNCH_SPOT ||
           second == kag::SHOP_FARMERS_MARKET)));
    if (parameters_.mode_rule >= 6 && third_pizza_cluster)
        return true;
    const bool third_brunch_farmers_cluster =
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET) &&
        ((first == kag::SHOP_PET_CAFE && second == kag::SHOP_PIZZA_SHOP) ||
         (first == kag::SHOP_YARN_STORE && second == kag::SHOP_PIZZA_SHOP) ||
         (first == kag::SHOP_PIZZA_SHOP &&
          (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE ||
           second == kag::SHOP_YARN_STORE)));
    if (parameters_.mode_rule >= 7 && third_brunch_farmers_cluster)
        return true;
    const bool third_yarn_cluster = first == kag::SHOP_YARN_STORE &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET);
    if (parameters_.mode_rule >= 8 && third_yarn_cluster)
        return true;
    if (parameters_.mode_rule == 2) return third_rule;
    const bool delayed_smoothie = parameters_.mode_rule >= 21 &&
        first == kag::SHOP_ICE_CREAM_SHOP && third_milk;
    const bool delayed_pizza_pair_milk = parameters_.mode_rule >= 22 &&
        first == kag::SHOP_PIZZA_SHOP &&
        second == kag::SHOP_PIZZA_SHOP && third_milk;
    if (third_rule)
        return (!delayed_smoothie && !delayed_pizza_pair_milk) ||
            observation.step >= 288;
    if (observation.n_shops < 4) return false;
    const int fourth = observation.shops[3];
    const bool fourth_milk = fourth == kag::SHOP_ICE_CREAM_SHOP ||
        fourth == kag::SHOP_SMOOTHIE_SHOP;
    const bool first_tail_family = first == kag::SHOP_BAKERY ||
        first == kag::SHOP_BRUNCH_SPOT ||
        first == kag::SHOP_FARMERS_MARKET || first == kag::SHOP_PET_CAFE;
    const bool fourth_brunch_farmers =
        fourth == kag::SHOP_BRUNCH_SPOT ||
        fourth == kag::SHOP_FARMERS_MARKET;
    if (parameters_.mode_rule >= 27 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
        third == kag::SHOP_BRUNCH_SPOT && fourth == kag::SHOP_PIZZA_SHOP)
        return true;
    if (parameters_.mode_rule >= 28 && first == kag::SHOP_BAKERY &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
        third == kag::SHOP_FARMERS_MARKET &&
        fourth == kag::SHOP_PIZZA_SHOP)
        return true;
    const bool third_bakery_pet_yarn =
        third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
        third == kag::SHOP_YARN_STORE;
    const bool fourth_bakery_pet_yarn =
        fourth == kag::SHOP_BAKERY || fourth == kag::SHOP_PET_CAFE ||
        fourth == kag::SHOP_YARN_STORE;
    if (parameters_.mode_rule >= 29 && first == kag::SHOP_BRUNCH_SPOT &&
        second == kag::SHOP_YARN_STORE && third_bakery_pet_yarn &&
        fourth_bakery_pet_yarn)
        return true;
    if (parameters_.mode_rule >= 30 && first == kag::SHOP_FARMERS_MARKET &&
        second == kag::SHOP_YARN_STORE && third_bakery_pet_yarn &&
        fourth_bakery_pet_yarn &&
        !(third == kag::SHOP_PET_CAFE && fourth == kag::SHOP_PET_CAFE))
        return true;
    if (parameters_.mode_rule >= 31 && first == kag::SHOP_BAKERY &&
        second == kag::SHOP_BRUNCH_SPOT &&
        ((third == kag::SHOP_BAKERY &&
          (fourth == kag::SHOP_BAKERY || fourth == kag::SHOP_PET_CAFE)) ||
         (third == kag::SHOP_PET_CAFE && fourth == kag::SHOP_BAKERY)))
        return true;
    if ((parameters_.mode_rule == 17 || parameters_.mode_rule >= 23) &&
        first == kag::SHOP_PIZZA_SHOP &&
        second == kag::SHOP_PIZZA_SHOP &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET) &&
        (fourth_milk || fourth_brunch_farmers))
        return true;
    if (parameters_.mode_rule >= 4 && !first_tail_family)
        return false;
    const bool unsafe_late_pizza = first == kag::SHOP_PIZZA_SHOP &&
        second == kag::SHOP_PIZZA_SHOP && third == kag::SHOP_PIZZA_SHOP;
    if (parameters_.mode_rule >= 10 && first == kag::SHOP_BAKERY &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) && third_milk)
        return true;
    if (parameters_.mode_rule >= 11 &&
        (first == kag::SHOP_BRUNCH_SPOT ||
         first == kag::SHOP_FARMERS_MARKET) &&
        second == kag::SHOP_YARN_STORE &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET))
        return true;
    if (parameters_.mode_rule >= 12 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (third == kag::SHOP_BRUNCH_SPOT ||
         third == kag::SHOP_FARMERS_MARKET) && fourth_brunch_farmers)
        return true;
    if (parameters_.mode_rule >= 13 &&
        (first == kag::SHOP_BRUNCH_SPOT ||
         first == kag::SHOP_FARMERS_MARKET) &&
        second == kag::SHOP_YARN_STORE &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
         third == kag::SHOP_YARN_STORE) && fourth_brunch_farmers)
        return true;
    const bool third_bakery_pet = third == kag::SHOP_BAKERY ||
        third == kag::SHOP_PET_CAFE;
    if (parameters_.mode_rule >= 14 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) && third_bakery_pet &&
        fourth_brunch_farmers)
        return true;
    if (parameters_.mode_rule >= 15 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) && third_bakery_pet &&
        fourth == kag::SHOP_PIZZA_SHOP)
        return true;
    if (parameters_.mode_rule >= 16 &&
        (first == kag::SHOP_BRUNCH_SPOT ||
         first == kag::SHOP_FARMERS_MARKET) &&
        second == kag::SHOP_YARN_STORE &&
        (third == kag::SHOP_BAKERY || third == kag::SHOP_PET_CAFE ||
         third == kag::SHOP_YARN_STORE) &&
        fourth == kag::SHOP_PIZZA_SHOP)
        return true;
    if (parameters_.mode_rule >= 18 &&
        (first == kag::SHOP_BRUNCH_SPOT ||
         first == kag::SHOP_FARMERS_MARKET) &&
        (second == kag::SHOP_BAKERY || second == kag::SHOP_PET_CAFE) &&
        third == kag::SHOP_YARN_STORE && !fourth_milk)
        return true;
    if (parameters_.mode_rule >= 19 &&
        (first == kag::SHOP_BAKERY || first == kag::SHOP_PET_CAFE) &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) &&
        third == kag::SHOP_YARN_STORE &&
        (fourth_brunch_farmers || fourth == kag::SHOP_PIZZA_SHOP))
        return true;
    if (parameters_.mode_rule >= 9 && first == kag::SHOP_BAKERY &&
        (second == kag::SHOP_BRUNCH_SPOT ||
         second == kag::SHOP_FARMERS_MARKET) && third_milk &&
        fourth_brunch_farmers)
        return true;
    return fourth_milk && !unsafe_yarn && !unsafe_late_pizza;
}

void Agent::reset(const kag::agent::AgentInit& init) {
    configure();
    base_.reset(init);
    if (parameters_.forced_mode != 255) mode_.reset(init);
    if (parameters_.third_yarn_rule || parameters_.fourth_yarn_rule)
        yarn_.reset(init);
    if (parameters_.third_pizza_rule || parameters_.fourth_pizza_rule)
        pizza_.reset(init);
    if (parameters_.smoothie_rule) smoothie_->reset(init);
    repair_activations_ = 0;
    initialized_ = true;
}

void Agent::repair_unaffordable_reveal_buy(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (observation.step != 216 || observation.n_shops < 3 ||
        observation.shops[0] != kag::SHOP_PIZZA_SHOP ||
        observation.shops[1] != kag::SHOP_PIZZA_SHOP ||
        (observation.shops[2] != kag::SHOP_ICE_CREAM_SHOP &&
         observation.shops[2] != kag::SHOP_PIZZA_SHOP &&
         observation.shops[2] != kag::SHOP_SMOOTHIE_SHOP))
        return;
    int cash = static_cast<int>(observation.self().money);
    bool changed = false;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        int price = 0;
        if (current.op == kag::M_BUY_SEED && current.item < kag::N_CROPS)
            price = kag::CROPS[current.item].seed;
        else if (current.op == kag::M_BUY_ANIMAL &&
                 current.item >= kag::GOOSE && current.item < kag::N_ITEMS)
            price = kag::ANIMALS[current.item - kag::GOOSE].cost;
        if (!price) continue;
        const int affordable = std::min(current.n, cash / price);
        cash -= affordable * price;
        if (current.op == kag::M_BUY_ANIMAL && affordable != current.n) {
            current.n = affordable;
            changed = true;
        }
    }
    if (!changed) return;
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    action.finalize();
    ++repair_activations_;
}

void Agent::repair_day_end_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    const bool mode_boundary = observation.step == 239 && use_mode(observation);
    const bool pizza_pet_yarn_boundary =
        parameters_.fourth_pizza_rule >= 16 && observation.step == 407 &&
        observation.n_shops >= 3 &&
        observation.shops[0] == kag::SHOP_PIZZA_SHOP &&
        observation.shops[1] == kag::SHOP_PET_CAFE &&
        observation.shops[2] == kag::SHOP_YARN_STORE;
    if ((observation.step != 263 && !mode_boundary &&
         !pizza_pet_yarn_boundary) ||
        observation.n_shops < 3)
        return;
    int projected = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int item = 0; item < kag::N_ITEMS; ++item)
            projected += observation.own.inv[unit][item];
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        const kag::UnitAction& current = action.units[unit];
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        const kag::Tile& tile = observation.self().tiles[y][x];
        if (current.op == kag::OP_HARVEST && tile.yield_units > 0 &&
            ((tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS &&
              observation.day - tile.planted_day >=
                  kag::CROPS[tile.what].first_yield_day) ||
             tile.has_animal))
            projected += tile.yield_units;
        else if (current.op == kag::OP_COLLECT_FERTILIZER &&
                 tile.has_animal && tile.fertilizer_available)
            ++projected;
        else if (current.op == kag::OP_FEED && tile.has_animal &&
                 !tile.fed_today && observation.own.inv[unit][kag::WHEAT])
            --projected;
        else if (current.op == kag::OP_FERTILIZE &&
                 tile.kind == kag::T_PLANT &&
                 observation.own.inv[unit][kag::FERTILIZER])
            --projected;
        else if (current.op == kag::OP_PLACE &&
                 current.arg >= kag::GOOSE && current.arg < kag::N_ITEMS &&
                 observation.own.inv[unit][current.arg])
            --projected;
    }
    std::array<int, kag::N_ITEMS> available{};
    for (int item = 0; item < kag::N_ITEMS; ++item)
        available[item] = observation.own.shed[item];
    for (int order = 0; order < action.n_orders; ++order) {
        const kag::Order& current = action.orders[order];
        if (current.op == kag::M_SELL && current.item < kag::N_ITEMS) {
            const int sold = std::min<int>(available[current.item], current.n);
            available[current.item] -= sold;
            projected -= sold;
        } else if ((current.op == kag::M_BUY_PRODUCT ||
                    current.op == kag::M_BUY_ANIMAL) &&
                   current.item < kag::N_ITEMS) {
            projected += current.n;
            available[current.item] += current.n;
        }
    }
    int overflow = projected - 100;
    if (overflow <= 0) return;
    constexpr std::array<int, kag::N_PRODUCTS> priority = {
        kag::FERTILIZER, kag::WHEAT, kag::EGG, kag::CARROT, kag::TOMATO,
        kag::STRAWBERRY, kag::MILK, kag::WOOL, kag::MELON};
    const int initial_overflow = overflow;
    for (int item : priority) {
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
    if (overflow == initial_overflow) return;
    action.finalize();
    ++repair_activations_;
}

void Agent::repair_day_end_cargo_capacity(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (observation.step % 24 != 23 || parameters_.mode_rule < 23 ||
        (parameters_.mode_rule != 24 && observation.step != 695))
        return;
    int projected = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        const kag::UnitAction& current = action.units[unit];
        if (current.op == kag::OP_PICKUP || current.op == kag::OP_DROP ||
            (current.op == kag::OP_PLACE &&
             current.arg < kag::N_PRODUCTS))
            return;
        for (int item = 0; item < kag::N_ITEMS; ++item)
            projected += observation.own.inv[unit][item];
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        const kag::Tile& tile = observation.self().tiles[y][x];
        if (current.op == kag::OP_HARVEST && tile.yield_units > 0)
            projected += tile.yield_units;
        else if (current.op == kag::OP_COLLECT_FERTILIZER &&
                 tile.has_animal && tile.fertilizer_available)
            ++projected;
        else if (current.op == kag::OP_FEED && tile.has_animal &&
                 !tile.fed_today && observation.own.inv[unit][kag::WHEAT])
            --projected;
        else if (current.op == kag::OP_FERTILIZE &&
                 tile.kind == kag::T_PLANT &&
                 observation.own.inv[unit][kag::FERTILIZER])
            --projected;
        else if (current.op == kag::OP_PLACE &&
                 current.arg >= kag::GOOSE && current.arg < kag::N_ITEMS &&
                 observation.own.inv[unit][current.arg])
            --projected;
    }
    std::array<int, kag::N_ITEMS> available{};
    for (int item = 0; item < kag::N_ITEMS; ++item)
        available[item] = observation.own.shed[item];
    for (int order = 0; order < action.n_orders; ++order) {
        const kag::Order& current = action.orders[order];
        if (current.op == kag::M_SELL && current.item < kag::N_ITEMS) {
            const int sold = std::min<int>(available[current.item], current.n);
            available[current.item] -= sold;
            projected -= sold;
        } else if (current.op == kag::M_BUY_PRODUCT ||
                   current.op == kag::M_BUY_ANIMAL) {
            return;
        }
    }
    int overflow = projected - 100;
    if (overflow <= 0 || overflow > 100 - observation.own.shed_total) return;
    kag::Action repaired = action;
    for (int unit = 0; unit < observation.self().n_units && overflow; ++unit) {
        if (repaired.units[unit].op != kag::OP_PASS ||
            !observation.own.inv[unit][kag::FERTILIZER])
            continue;
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        if (observation.self().tiles[y][x].kind != kag::T_PLANT) continue;
        repaired.units[unit] = {kag::OP_FERTILIZE, 0, 1};
        --overflow;
    }
    if (!overflow) {
        repaired.finalize();
        action = repaired;
        ++repair_activations_;
        return;
    }
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        if (repaired.units[unit].op != kag::OP_PASS ||
            !kag::is_shed_adjacent(observation.self().pos_x[unit],
                                   observation.self().pos_y[unit], kag::BOARD))
            continue;
        for (int item = 0; item < kag::N_PRODUCTS; ++item) {
            if (observation.own.inv[unit][item] < overflow) continue;
            for (int order = 0; order < repaired.n_orders; ++order) {
                kag::Order& sale = repaired.orders[order];
                if (sale.op != kag::M_SELL || sale.item != item) continue;
                repaired.units[unit] = {
                    kag::OP_PLACE, static_cast<uint8_t>(item), overflow};
                sale.n += overflow;
                repaired.finalize();
                action = repaired;
                ++repair_activations_;
                return;
            }
        }
    }
}

void Agent::repair_terminal_cargo(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!parameters_.terminal_cargo_repair || observation.step != 718 ||
        (!use_mode(observation) && !use_pizza(observation)))
        return;
    std::array<int, kag::N_ITEMS> cargo{};
    int cargo_total = 0;
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        int unit_total = 0;
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            cargo[item] += observation.own.inv[unit][item];
            unit_total += observation.own.inv[unit][item];
        }
        if (unit_total && !kag::is_shed_adjacent(
                observation.self().pos_x[unit],
                observation.self().pos_y[unit], kag::BOARD))
            return;
        cargo_total += unit_total;
    }
    if (!cargo_total || cargo_total > 100 - observation.own.shed_total)
        return;
    for (int unit = 0; unit < action.n_units; ++unit)
        action.units[unit] = {};
    for (int unit = 0; unit < action.n_units; ++unit) {
        int unit_total = 0;
        for (int item = 0; item < kag::N_ITEMS; ++item)
            unit_total += observation.own.inv[unit][item];
        if (unit_total) action.units[unit] = {kag::OP_DROP, 0, 1};
    }
    action.n_orders = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const int quantity = observation.own.shed[item] + cargo[item];
        if (quantity)
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item), quantity};
    }
    action.finalize();
    ++repair_activations_;
}

void Agent::repair_terminal_seed_buys(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!parameters_.terminal_seed_mask || !use_mode(observation) ||
        observation.n_shops < 2)
        return;
    const bool base_already_repairs =
        (observation.shops[0] == kag::SHOP_BRUNCH_SPOT ||
         observation.shops[0] == kag::SHOP_FARMERS_MARKET) &&
        observation.shops[1] != kag::SHOP_YARN_STORE;
    if (base_already_repairs) return;
    int crop = -1;
    int reduction = 0;
    uint8_t bit = 0;
    if (observation.shops[0] == kag::SHOP_YARN_STORE &&
        observation.step == 528) {
        crop = kag::WHEAT;
        reduction = 5;
        bit = uint8_t{1} << 0;
    } else if (observation.step == 360) {
        crop = kag::TOMATO;
        reduction = 1;
        bit = uint8_t{1} << 1;
    } else if (observation.step == 552) {
        crop = kag::WHEAT;
        reduction = 5;
        bit = uint8_t{1} << 2;
    } else if (observation.step == 576) {
        crop = kag::WHEAT;
        reduction = 7;
        bit = uint8_t{1} << 3;
    } else if (observation.step == 600) {
        crop = kag::WHEAT;
        reduction = 2;
        bit = uint8_t{1} << 4;
    } else if (observation.step == 624) {
        crop = kag::CARROT;
        reduction = 5;
        bit = uint8_t{1} << 5;
    }
    if (crop < 0 || !(parameters_.terminal_seed_mask & bit)) return;
    bool changed = false;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op != kag::M_BUY_SEED || current.item != crop) continue;
        const int removed = std::min(current.n, reduction);
        current.n -= removed;
        reduction -= removed;
        changed |= removed > 0;
    }
    if (!changed) return;
    int write = 0;
    for (int read = 0; read < action.n_orders; ++read)
        if (action.orders[read].n > 0)
            action.orders[write++] = action.orders[read];
    action.n_orders = write;
    action.finalize();
    ++repair_activations_;
}

void Agent::adjust_pet_day11_melon_sale(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (parameters_.pet_day11_melon_sale == 255 ||
        observation.step != 264 || observation.n_shops < 3 ||
        observation.shops[0] != kag::SHOP_PET_CAFE ||
        (observation.shops[1] != kag::SHOP_BAKERY &&
         observation.shops[1] != kag::SHOP_PET_CAFE) ||
        (observation.shops[2] != kag::SHOP_BAKERY &&
         observation.shops[2] != kag::SHOP_PET_CAFE))
        return;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op != kag::M_SELL || current.item != kag::MELON) continue;
        current.n = parameters_.pet_day11_melon_sale;
        action.finalize();
        return;
    }
}

void Agent::adjust_pet_wheat_timing(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!parameters_.pet_day11_wheat_shift || observation.n_shops < 3 ||
        observation.shops[0] != kag::SHOP_PET_CAFE ||
        (observation.shops[1] != kag::SHOP_BAKERY &&
         observation.shops[1] != kag::SHOP_PET_CAFE) ||
        (observation.shops[2] != kag::SHOP_BAKERY &&
         observation.shops[2] != kag::SHOP_PET_CAFE))
        return;
    if (observation.step == 264) {
        int first_hire = -1;
        int hires = 0;
        int write = 0;
        for (int read = 0; read < action.n_orders; ++read) {
            const kag::Order current = action.orders[read];
            if (current.op == kag::M_HIRE) {
                if (first_hire < 0) first_hire = write++;
                hires += current.n;
            } else {
                action.orders[write++] = current;
            }
        }
        if (first_hire < 0 || write >= 10) return;
        action.orders[first_hire] = {kag::M_HIRE, 0, hires};
        action.n_orders = write;
        action.orders[action.n_orders++] = {
            kag::M_BUY_PRODUCT, kag::WHEAT,
            parameters_.pet_day11_wheat_shift};
        action.finalize();
        return;
    }
    if (observation.step != 288) return;
    for (int order = 0; order < action.n_orders; ++order) {
        kag::Order& current = action.orders[order];
        if (current.op != kag::M_BUY_PRODUCT || current.item != kag::WHEAT)
            continue;
        current.n = std::max<int>(
            0, current.n - parameters_.pet_day11_wheat_shift);
        int write = 0;
        for (int read = 0; read < action.n_orders; ++read)
            if (action.orders[read].n > 0)
                action.orders[write++] = action.orders[read];
        action.n_orders = write;
        action.finalize();
        return;
    }
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    if (!initialized_) std::abort();
    base_.act(observation, budget, action);
    kag::Action yarn_action;
    bool yarn_selected = false;
    if (parameters_.third_yarn_rule || parameters_.fourth_yarn_rule) {
        yarn_.act(observation, budget, yarn_action);
        yarn_selected = use_yarn(observation);
        if (yarn_selected) action = yarn_action;
    }
    if (parameters_.forced_mode != 255) {
        kag::Action mode_action;
        mode_.act(observation, budget, mode_action);
        if (use_mode(observation)) action = mode_action;
    }
    if (yarn_selected && observation.shops[2] != kag::SHOP_YARN_STORE)
        action = yarn_action;
    if (parameters_.third_pizza_rule || parameters_.fourth_pizza_rule) {
        kag::Action pizza_action;
        pizza_.act(observation, budget, pizza_action);
        if (use_pizza(observation)) action = pizza_action;
    }
    if (parameters_.smoothie_rule) {
        kag::Action smoothie_action;
        smoothie_->act(observation, budget, smoothie_action);
        if (use_smoothie(observation)) action = smoothie_action;
    }
    adjust_pet_day11_melon_sale(observation, action);
    adjust_pet_wheat_timing(observation, action);
    repair_unaffordable_reveal_buy(observation, action);
    repair_terminal_seed_buys(observation, action);
    repair_day_end_capacity(observation, action);
    repair_day_end_cargo_capacity(observation, action);
    repair_terminal_cargo(observation, action);
}

}  // namespace kag::agents::four_random_shop
