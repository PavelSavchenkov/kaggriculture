#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/agent.hpp"

#include <algorithm>
#include <cstdlib>
#include <array>
#include <utility>
#include <variant>

#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/bakery_108320_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/farmers_market_114557_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/ice_cream_143849_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pet_cafe_107453_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pet_cafe_107453_robust/core.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pet_cafe_107453_robust/donor_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pet_cafe_107453_robust/guard.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pet_cafe_107453_robust/reassignment_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/pizza_125556_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/smoothie_142388_robust/agent.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/yarn_125268_robust/agent.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::one_random_shop_dynamic_robust {
namespace {

class PetNoResidual {
public:
    PetNoResidual() {
        pet_cafe_107453_robust::detail::RepairParameters repair;
        repair.cleanup_lookahead_days = 4;
        repair.cleanup_lookahead_kind_mask = uint32_t{1} << kag::WHEAT;
        repair.liquidity_sales = true;
        repair.liquidity_opportunity_selector = true;
        repair.underfilled_sale_substitution = true;
        core_.set_parameters(repair);
        donor_.set_parameters({true,
            (uint32_t{1} << 11) | (uint32_t{1} << 17) |
                (uint32_t{1} << 19),
            (uint32_t{1} << kag::WHEAT) | (uint32_t{1} << kag::MELON) |
                (uint32_t{1} << (kag::N_CROPS + 1)),
            0, false, 5});
        reassignment_.set_parameters({true,
            (uint32_t{1} << 11) | (uint32_t{1} << 17) |
                (uint32_t{1} << 19),
            0});
    }

    void reset(const kag::agent::AgentInit& init) {
        core_.reset(init);
        donor_.reset(init.player);
        reassignment_.reset(init.player);
    }

    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action) {
        core_.act(observation, budget, action);
        donor_.modify(observation, action);
        reassignment_.modify(observation, action);
        kag::pet_cafe_portfolio::guard_observation(observation, action);
    }

private:
    pet_cafe_107453_robust::detail::CoreAgent core_;
    pet_cafe_107453_robust::detail::donor::DonorOverlay donor_;
    pet_cafe_107453_robust::detail::reassignment::Overlay reassignment_;
};

using Delegate = std::variant<
    bakery_108320_robust::Agent,
    brunch_114643_robust::Agent,
    farmers_market_114557_robust::Agent,
    ice_cream_143849_robust::Agent,
    PetNoResidual,
    pizza_125556_robust::Agent,
    smoothie_142388_robust::Agent,
    yarn_125268_robust::Agent>;

void select(Delegate& delegate, int shop) {
    switch (shop) {
        case kag::SHOP_BAKERY: delegate.emplace<0>(); break;
        case kag::SHOP_BRUNCH_SPOT: delegate.emplace<1>(); break;
        case kag::SHOP_FARMERS_MARKET: delegate.emplace<2>(); break;
        case kag::SHOP_ICE_CREAM_SHOP: delegate.emplace<3>(); break;
        case kag::SHOP_PET_CAFE: delegate.emplace<4>(); break;
        case kag::SHOP_PIZZA_SHOP: delegate.emplace<5>(); break;
        case kag::SHOP_SMOOTHIE_SHOP: delegate.emplace<6>(); break;
        case kag::SHOP_YARN_STORE: delegate.emplace<7>(); break;
        default: std::abort();
    }
}

bool set_order_quantity(kag::Action& action, uint8_t op, uint8_t item,
                        int quantity, int wanted_occurrence = 0) {
    int occurrence = 0;
    for (int order = 0; order < action.n_orders; ++order) {
        if (action.orders[order].op != op || action.orders[order].item != item)
            continue;
        if (occurrence++ != wanted_occurrence) continue;
        if (quantity > 0) {
            action.orders[order].n = quantity;
        } else {
            for (int next = order; next + 1 < action.n_orders; ++next)
                action.orders[next] = action.orders[next + 1];
            --action.n_orders;
        }
        action.finalize();
        return true;
    }
    return false;
}

void remove_sale_units(kag::Action& action, uint8_t item, int quantity) {
    for (int order = action.n_orders - 1;
         order >= 0 && quantity > 0; --order) {
        if (action.orders[order].op != kag::M_SELL ||
            action.orders[order].item != item)
            continue;
        const int removed = std::min<int>(action.orders[order].n, quantity);
        action.orders[order].n -= removed;
        quantity -= removed;
    }
    action.finalize();
}

}  // namespace

struct Agent::Impl {
    Delegate delegate;
    bakery_108320_robust::Agent bakery_shadow;
    brunch_114643_robust::Agent brunch_shadow;
    farmers_market_114557_robust::Agent farmers_shadow;
    ice_cream_143849_robust::Agent ice_shadow;
    PetNoResidual pet_shadow;
    pizza_125556_robust::Agent pizza_shadow;
    smoothie_142388_robust::Agent smoothie_shadow;
    yarn_125268_robust::Agent yarn_shadow;
    kag::agent::AgentInit init{};
    int opening_shop = kag::SHOP_BAKERY;
    int second_opening_shop = kag::SHOP_BAKERY;
    int opening_cut_step = 72;
    std::array<uint8_t, 72> opening_sources{};
    int active_shop = -1;
    std::array<int, kag::N_SHOPS> branch_shop = {
        kag::SHOP_BAKERY, kag::SHOP_BRUNCH_SPOT,
        kag::SHOP_FARMERS_MARKET, kag::SHOP_ICE_CREAM_SHOP,
        kag::SHOP_PET_CAFE, kag::SHOP_PIZZA_SHOP,
        kag::SHOP_SMOOTHIE_SHOP, kag::SHOP_YARN_STORE};
    bool initialized = false;
    bool shadow_rejoin = true;
    bool flexible_animal = false;
    int opening_fertilizer_sale = 4;
    int flexible_worker = -1;
    bool yarn_repair = true;
    int yarn_courier = -1;
    bool yarn_sheep_installed = false;
    int yarn_held_wool = 0;
    int pizza_reveal_wheat = 5;
    bool pet_reveal_wheat = true;
    int pizza_day7_fertilizer = 1;
    int pizza_day12_wheat = 18;
    int pizza_step383_milk = 0;
    int pizza_day20_wheat_sale = 0;
    int pizza_step695_second_milk = 10;

    void reset_shadows() {
        bakery_shadow.reset(init);
        brunch_shadow.reset(init);
        farmers_shadow.reset(init);
        ice_shadow.reset(init);
        pet_shadow.reset(init);
        pizza_shadow.reset(init);
        smoothie_shadow.reset(init);
        yarn_shadow.reset(init);
    }

    template<class Shadow>
    static void act_candidate(Shadow& shadow, int id, int selected,
                              const kag::agent::AgentObservation& observation,
                              const kag::agent::DecisionBudget& budget,
                              kag::Action& action, kag::Action& scratch) {
        shadow.act(observation, budget, id == selected ? action : scratch);
    }

    void act_opening(int selected,
                     const kag::agent::AgentObservation& observation,
                     const kag::agent::DecisionBudget& budget,
                     kag::Action& action) {
        kag::Action scratch;
        act_candidate(bakery_shadow, kag::SHOP_BAKERY, selected,
                      observation, budget, action, scratch);
        act_candidate(brunch_shadow, kag::SHOP_BRUNCH_SPOT, selected,
                      observation, budget, action, scratch);
        act_candidate(farmers_shadow, kag::SHOP_FARMERS_MARKET, selected,
                      observation, budget, action, scratch);
        act_candidate(ice_shadow, kag::SHOP_ICE_CREAM_SHOP, selected,
                      observation, budget, action, scratch);
        act_candidate(pet_shadow, kag::SHOP_PET_CAFE, selected,
                      observation, budget, action, scratch);
        act_candidate(pizza_shadow, kag::SHOP_PIZZA_SHOP, selected,
                      observation, budget, action, scratch);
        act_candidate(smoothie_shadow, kag::SHOP_SMOOTHIE_SHOP, selected,
                      observation, budget, action, scratch);
        act_candidate(yarn_shadow, kag::SHOP_YARN_STORE, selected,
                      observation, budget, action, scratch);
    }

    void act_shadow(int shop,
                    const kag::agent::AgentObservation& observation,
                    const kag::agent::DecisionBudget& budget,
                    kag::Action& action) {
        switch (shop) {
            case kag::SHOP_BAKERY:
                bakery_shadow.act(observation, budget, action); break;
            case kag::SHOP_BRUNCH_SPOT:
                brunch_shadow.act(observation, budget, action); break;
            case kag::SHOP_FARMERS_MARKET:
                farmers_shadow.act(observation, budget, action); break;
            case kag::SHOP_ICE_CREAM_SHOP:
                ice_shadow.act(observation, budget, action); break;
            case kag::SHOP_PET_CAFE:
                pet_shadow.act(observation, budget, action); break;
            case kag::SHOP_PIZZA_SHOP:
                pizza_shadow.act(observation, budget, action); break;
            case kag::SHOP_SMOOTHIE_SHOP:
                smoothie_shadow.act(observation, budget, action); break;
            case kag::SHOP_YARN_STORE:
                yarn_shadow.act(observation, budget, action); break;
            default: std::abort();
        }
    }

    void repair_yarn(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
        const kag::Tile& target = observation.self().tiles[3][1];
        if (observation.day <= 4 && target.has_animal &&
            target.what == kag::COW) {
            for (int unit = 0; unit < action.n_units; ++unit)
                if (observation.self().pos_x[unit] == 1 &&
                    observation.self().pos_y[unit] == 3 &&
                    action.units[unit].op == kag::OP_FEED)
                    action.units[unit] = {};
        }
        if (observation.step == 144 && !target.has_animal &&
            target.kind == kag::T_PASTURE &&
            observation.own.shed[kag::FERTILIZER] >= 6 &&
            observation.own.shed[kag::WHEAT] >= 1 &&
            action.n_orders <= 8) {
            bool fertilizer_sale = false;
            for (int order = 0; order < action.n_orders; ++order)
                if (action.orders[order].op == kag::M_SELL &&
                    action.orders[order].item == kag::FERTILIZER) {
                    action.orders[order].n = 6;
                    fertilizer_sale = true;
                }
            if (fertilizer_sale) {
                action.orders[action.n_orders++] = {
                    kag::M_SELL, kag::WHEAT, 1};
                action.orders[action.n_orders++] = {
                    kag::M_BUY_ANIMAL, kag::SHEEP, 1};
            }
        }
        if (target.has_animal && target.what == kag::SHEEP) {
            yarn_sheep_installed = true;
            yarn_courier = -1;
        }
        if (observation.step == 337) {
            int remaining = 4;
            for (int order = action.n_orders - 1;
                 order >= 0 && remaining > 0; --order) {
                if (action.orders[order].op != kag::M_SELL ||
                    action.orders[order].item != kag::WOOL)
                    continue;
                const int removed = std::min<int>(action.orders[order].n,
                                                  remaining);
                action.orders[order].n -= removed;
                remaining -= removed;
                yarn_held_wool += removed;
            }
        }
        if (observation.step == 504 && yarn_held_wool > 0) {
            for (int order = 0; order < action.n_orders; ++order)
                if (action.orders[order].op == kag::M_SELL &&
                    action.orders[order].item == kag::WOOL) {
                    action.orders[order].n = std::min<int>(
                        action.orders[order].n + yarn_held_wool,
                        observation.own.shed[kag::WOOL]);
                    yarn_held_wool = 0;
                    break;
                }
        }
        const bool courier_has_sheep = yarn_courier >= 0 &&
            yarn_courier < action.n_units &&
            observation.own.inv[yarn_courier][kag::SHEEP] > 0;
        if (!yarn_sheep_installed && observation.day == 6 &&
            (observation.own.shed[kag::SHEEP] > 0 || courier_has_sheep)) {
            if (yarn_courier < 0 || yarn_courier >= action.n_units) {
                for (int unit = 0; unit < action.n_units; ++unit)
                    if (observation.self().pos_x[unit] == 5 &&
                        observation.self().pos_y[unit] == 4) {
                        yarn_courier = unit;
                        break;
                    }
            }
            if (yarn_courier >= 0 && yarn_courier < action.n_units) {
                const int x = observation.self().pos_x[yarn_courier];
                const int y = observation.self().pos_y[yarn_courier];
                if (observation.own.inv[yarn_courier][kag::SHEEP] > 0) {
                    if (x == 1 && y == 3 && !target.has_animal &&
                        target.kind == kag::T_PASTURE)
                        action.units[yarn_courier] = {
                            kag::OP_PLACE, kag::SHEEP, 1};
                    else if (x == 1 && y == 4 &&
                             observation.self().tiles[4][1].kind ==
                                 kag::T_PLANT &&
                             !observation.self().tiles[4][1].watered_today)
                        action.units[yarn_courier] = {kag::OP_WATER, 0, 1};
                    else if (x > 1)
                        action.units[yarn_courier] = {kag::OP_WEST, 0, 1};
                    else if (x < 1)
                        action.units[yarn_courier] = {kag::OP_EAST, 0, 1};
                    else if (y > 3)
                        action.units[yarn_courier] = {kag::OP_NORTH, 0, 1};
                    else if (y < 3)
                        action.units[yarn_courier] = {kag::OP_SOUTH, 0, 1};
                } else if (kag::is_shed_adjacent(x, y, kag::BOARD)) {
                    action.units[yarn_courier] = {
                        kag::OP_PICKUP, kag::SHEEP, 1};
                }
            }
        }
        action.finalize();
    }
};

Agent::Agent() : impl_(std::make_unique<Impl>()) {}
Agent::~Agent() = default;
Agent::Agent(Agent&&) noexcept = default;
Agent& Agent::operator=(Agent&&) noexcept = default;

kag::agent::AgentInfo Agent::info() { return {"one_random_shop_dynamic_robust"}; }

void Agent::set_opening_shop(int shop) {
    if (!impl_ || shop < 0 || shop >= kag::N_SHOPS || impl_->initialized)
        std::abort();
    impl_->opening_shop = shop;
    impl_->second_opening_shop = shop;
    impl_->opening_sources.fill(static_cast<uint8_t>(shop));
}

void Agent::set_opening_schedule(int first_shop, int second_shop, int cut_step) {
    if (!impl_ || first_shop < 0 || first_shop >= kag::N_SHOPS ||
        second_shop < 0 || second_shop >= kag::N_SHOPS || cut_step < 0 ||
        cut_step > 72 || impl_->initialized)
        std::abort();
    impl_->opening_shop = first_shop;
    impl_->second_opening_shop = second_shop;
    impl_->opening_cut_step = cut_step;
    for (int step = 0; step < 72; ++step)
        impl_->opening_sources[step] = static_cast<uint8_t>(
            step < cut_step ? first_shop : second_shop);
}

void Agent::set_opening_sources(const std::array<uint8_t, 72>& sources) {
    if (!impl_ || impl_->initialized) std::abort();
    for (uint8_t source : sources)
        if (source >= kag::N_SHOPS) std::abort();
    impl_->opening_shop = sources[0];
    impl_->second_opening_shop = sources[71];
    impl_->opening_sources = sources;
}

void Agent::set_branch_shop(int observed_shop, int delegate_shop) {
    if (!impl_ || observed_shop < 0 || observed_shop >= kag::N_SHOPS ||
        delegate_shop < 0 || delegate_shop >= kag::N_SHOPS ||
        impl_->initialized)
        std::abort();
    impl_->branch_shop[observed_shop] = delegate_shop;
}

void Agent::set_shadow_rejoin(bool enabled) {
    if (!impl_ || impl_->initialized) std::abort();
    impl_->shadow_rejoin = enabled;
}

void Agent::set_flexible_animal(bool enabled) {
    if (!impl_ || impl_->initialized) std::abort();
    impl_->flexible_animal = enabled;
}

void Agent::set_opening_fertilizer_sale(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 20)
        std::abort();
    impl_->opening_fertilizer_sale = quantity;
}

void Agent::set_yarn_repair(bool enabled) {
    if (!impl_ || impl_->initialized) std::abort();
    impl_->yarn_repair = enabled;
}

void Agent::set_pizza_reveal_wheat(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 20)
        std::abort();
    impl_->pizza_reveal_wheat = quantity;
}

void Agent::set_pet_reveal_wheat(bool enabled) {
    if (!impl_ || impl_->initialized) std::abort();
    impl_->pet_reveal_wheat = enabled;
}

void Agent::set_pizza_day7_fertilizer(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 20)
        std::abort();
    impl_->pizza_day7_fertilizer = quantity;
}

void Agent::set_pizza_day12_wheat(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 50)
        std::abort();
    impl_->pizza_day12_wheat = quantity;
}

void Agent::set_pizza_step383_milk(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 50)
        std::abort();
    impl_->pizza_step383_milk = quantity;
}

void Agent::set_pizza_day20_wheat_sale(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 50)
        std::abort();
    impl_->pizza_day20_wheat_sale = quantity;
}

void Agent::set_pizza_step695_second_milk(int quantity) {
    if (!impl_ || impl_->initialized || quantity < 0 || quantity > 50)
        std::abort();
    impl_->pizza_step695_second_milk = quantity;
}

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!impl_) std::abort();
    impl_->init = init;
    impl_->active_shop = impl_->opening_shop;
    impl_->flexible_worker = -1;
    impl_->yarn_courier = -1;
    impl_->yarn_sheep_installed = false;
    impl_->yarn_held_wool = 0;
    select(impl_->delegate, impl_->active_shop);
    std::visit([&](auto& agent) { agent.reset(init); }, impl_->delegate);
    if (impl_->shadow_rejoin) impl_->reset_shadows();
    impl_->initialized = true;
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget, kag::Action& action) {
    if (!impl_ || !impl_->initialized) std::abort();
    if (observation.n_shops == 0) {
        if (impl_->shadow_rejoin) {
            const int selected = impl_->opening_sources[observation.step];
            impl_->act_opening(selected, observation, budget, action);
            if (observation.step == 48 &&
                impl_->opening_fertilizer_sale >= 0) {
                bool found = false;
                for (int order = 0; order < action.n_orders; ++order)
                    if (action.orders[order].op == kag::M_SELL &&
                        action.orders[order].item == kag::FERTILIZER) {
                        action.orders[order].n =
                            impl_->opening_fertilizer_sale;
                        found = true;
                        break;
                    }
                if (!found) std::abort();
                action.finalize();
            }
        } else {
            std::visit([&](auto& agent) { agent.act(observation, budget, action); },
                       impl_->delegate);
        }
        if (impl_->flexible_animal) {
            if (observation.step == 24)
                for (int order = 0; order < action.n_orders; ++order)
                    if (action.orders[order].op == kag::M_BUY_ANIMAL &&
                        action.orders[order].item == kag::COW &&
                        action.orders[order].n == 1) {
                        for (int next = order; next + 1 < action.n_orders; ++next)
                            action.orders[next] = action.orders[next + 1];
                        --action.n_orders;
                        break;
                    }
            if (observation.step == 26 || observation.step == 28 ||
                observation.step == 29 || observation.step == 30)
                action.units[0] = {};
            action.finalize();
        }
        return;
    }
    if (observation.n_shops > 0) {
        const int revealed = observation.shops[0];
        if (revealed < 0 || revealed >= kag::N_SHOPS) std::abort();
        const int selected = impl_->branch_shop[revealed];
        if (impl_->shadow_rejoin) {
            impl_->act_shadow(selected, observation, budget, action);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 72 && impl_->pizza_reveal_wheat >= 0) {
                set_order_quantity(action, kag::M_BUY_PRODUCT, kag::WHEAT,
                                   impl_->pizza_reveal_wheat);
            }
            if (revealed == kag::SHOP_PET_CAFE &&
                selected == kag::SHOP_PET_CAFE && observation.step == 72 &&
                impl_->pet_reveal_wheat && action.n_orders < 10) {
                action.orders[action.n_orders++] = {
                    kag::M_BUY_PRODUCT, kag::WHEAT, 1};
                action.finalize();
            }
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 168) {
                set_order_quantity(action, kag::M_BUY_PRODUCT,
                                   kag::FERTILIZER,
                                   impl_->pizza_day7_fertilizer);
            }
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 288) {
                set_order_quantity(action, kag::M_BUY_PRODUCT, kag::WHEAT,
                                   impl_->pizza_day12_wheat);
            }
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 383) {
                set_order_quantity(action, kag::M_SELL, kag::MILK,
                                   impl_->pizza_step383_milk);
                set_order_quantity(action, kag::M_SELL, kag::MILK,
                                   impl_->pizza_step383_milk);
            }
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 480) {
                set_order_quantity(action, kag::M_SELL, kag::WHEAT,
                                   impl_->pizza_day20_wheat_sale);
            }
            if ((revealed == kag::SHOP_ICE_CREAM_SHOP ||
                 revealed == kag::SHOP_SMOOTHIE_SHOP) &&
                selected == revealed && observation.step == 677 &&
                observation.own.shed[kag::EGG] >= 26)
                remove_sale_units(action, kag::EGG, 7);
            if ((revealed == kag::SHOP_ICE_CREAM_SHOP ||
                 revealed == kag::SHOP_SMOOTHIE_SHOP) &&
                selected == revealed && observation.step == 694)
                remove_sale_units(action, kag::STRAWBERRY, 1);
            if (revealed == kag::SHOP_PET_CAFE &&
                selected == revealed && observation.step == 694)
                set_order_quantity(action, kag::M_SELL, kag::WOOL, 8);
            if (revealed == kag::SHOP_PET_CAFE &&
                selected == revealed && observation.step == 477)
                set_order_quantity(action, kag::M_SELL, kag::CARROT, 4);
            if (revealed == kag::SHOP_PET_CAFE &&
                selected == revealed && observation.step == 592)
                set_order_quantity(action, kag::M_SELL, kag::CARROT, 0);
            if (revealed == kag::SHOP_BAKERY &&
                selected == revealed && observation.step == 120)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 12);
            if (revealed == kag::SHOP_BAKERY &&
                selected == revealed && observation.step == 127)
                set_order_quantity(action, kag::M_SELL, kag::EGG, 0);
            if (revealed == kag::SHOP_BAKERY &&
                selected == revealed && observation.step == 158)
                set_order_quantity(action, kag::M_SELL, kag::FERTILIZER, 3);
            if (revealed == kag::SHOP_BRUNCH_SPOT &&
                selected == revealed && observation.step == 528)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 1);
            if (revealed == kag::SHOP_BRUNCH_SPOT &&
                selected == revealed && observation.step == 624)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 9);
            if (revealed == kag::SHOP_BRUNCH_SPOT &&
                selected == revealed && observation.step == 552)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 6);
            if (revealed == kag::SHOP_BRUNCH_SPOT &&
                selected == revealed && observation.step == 648)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 1);
            if (revealed == kag::SHOP_BRUNCH_SPOT &&
                selected == revealed && observation.step == 120)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 13);
            if (revealed == kag::SHOP_FARMERS_MARKET &&
                selected == revealed && observation.step == 528)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 0);
            if (revealed == kag::SHOP_FARMERS_MARKET &&
                selected == revealed && observation.step == 624)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 9);
            if (revealed == kag::SHOP_FARMERS_MARKET &&
                selected == revealed && observation.step == 552)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 6);
            if (revealed == kag::SHOP_FARMERS_MARKET &&
                selected == revealed && observation.step == 648)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 1);
            if (revealed == kag::SHOP_FARMERS_MARKET &&
                selected == revealed && observation.step == 120)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 12);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 528)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 0);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 624)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 7);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 360)
                set_order_quantity(action, kag::M_BUY_SEED, kag::TOMATO, 1);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 576)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 3);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 433)
                set_order_quantity(action, kag::M_BUY_PRODUCT, kag::WHEAT, 1);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 440)
                set_order_quantity(action, kag::M_BUY_PRODUCT, kag::WHEAT, 5);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 453)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 709)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 0);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 593)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 620)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 621)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 28);
            if (revealed == kag::SHOP_ICE_CREAM_SHOP &&
                selected == revealed && observation.step == 715)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 11);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 528)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 624)
                set_order_quantity(action, kag::M_BUY_SEED, kag::CARROT, 7);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 360)
                set_order_quantity(action, kag::M_BUY_SEED, kag::TOMATO, 1);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 576)
                set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT, 3);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 433)
                set_order_quantity(action, kag::M_BUY_PRODUCT, kag::WHEAT, 1);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 454)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 709)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 297)
                set_order_quantity(action, kag::M_SELL, kag::MELON, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 593)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 620)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 621)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 28);
            if (revealed == kag::SHOP_SMOOTHIE_SHOP &&
                selected == revealed && observation.step == 715)
                set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY, 11);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 697)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 697)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 551)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 673)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 716)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0, 1);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 716)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 647)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 9);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 648)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 455)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 4);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 648)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 359)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 384)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 1);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 335)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 407)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 599)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 6);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 600)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 576)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 360)
                set_order_quantity(action, kag::M_SELL, kag::WHEAT, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 311)
                set_order_quantity(action, kag::M_SELL, kag::MELON, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 287)
                set_order_quantity(action, kag::M_SELL, kag::MELON, 0, 1);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 480)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 241)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 503)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 14);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 239)
                set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == revealed && observation.step == 288)
                set_order_quantity(action, kag::M_SELL, kag::FERTILIZER, 23);
            if (revealed == kag::SHOP_PIZZA_SHOP &&
                selected == kag::SHOP_PIZZA_SHOP &&
                observation.step == 695) {
                set_order_quantity(action, kag::M_SELL, kag::MILK, 3);
                set_order_quantity(action, kag::M_SELL, kag::MILK,
                                   impl_->pizza_step695_second_milk, 1);
            }
            if (impl_->yarn_repair && revealed == kag::SHOP_YARN_STORE &&
                selected == kag::SHOP_YARN_STORE) {
                impl_->repair_yarn(observation, action);
                if (observation.step == 120)
                    set_order_quantity(action, kag::M_SELL, kag::WHEAT, 20);
                if (observation.step == 216)
                    set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT,
                                       2);
                if (observation.step == 217)
                    set_order_quantity(action, kag::M_SELL, kag::CARROT, 0);
                if (observation.step == 264)
                    set_order_quantity(action, kag::M_SELL, kag::MELON, 3);
                if (observation.step == 265)
                    set_order_quantity(action, kag::M_SELL, kag::MELON, 0);
                if (observation.step == 335)
                    set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
                if (observation.step == 336)
                    set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
                if (observation.step == 337)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 385)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 433)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 456)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 481)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 528)
                    set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
                if (observation.step == 528)
                    set_order_quantity(action, kag::M_SELL, kag::WHEAT, 5);
                if (observation.step == 529)
                    set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
                if (observation.step == 529)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 529)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 552)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 576)
                    set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY,
                                       0);
                if (observation.step == 577)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 600)
                    set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY,
                                       0);
                if (observation.step == 601)
                    set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY,
                                       0);
                if (observation.step == 432)
                    set_order_quantity(action, kag::M_BUY_SEED, kag::WHEAT,
                                       6);
                if (observation.step == 480)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 504)
                    set_order_quantity(action, kag::M_SELL, kag::MELON, 0);
                if (observation.step == 624)
                    set_order_quantity(action, kag::M_SELL, kag::MILK, 0);
                if (observation.step == 624)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
                if (observation.step == 673)
                    set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY,
                                       0);
                if (observation.step == 673)
                    set_order_quantity(action, kag::M_SELL, kag::MELON, 0);
                if (observation.step == 694)
                    set_order_quantity(action, kag::M_SELL, kag::STRAWBERRY,
                                       0);
                if (observation.step == 709)
                    set_order_quantity(action, kag::M_SELL, kag::WOOL, 0);
            }
            if (impl_->flexible_animal) {
                const int animal = revealed == kag::SHOP_YARN_STORE ?
                    kag::SHEEP : kag::COW;
                const kag::Tile& target = observation.self().tiles[4][4];
                if (observation.step == 72 && action.n_orders < 10) {
                    for (int order = action.n_orders; order > 0; --order)
                        action.orders[order] = action.orders[order - 1];
                    action.orders[0] = {
                        kag::M_BUY_ANIMAL, static_cast<uint8_t>(animal), 1};
                    ++action.n_orders;
                } else if (!target.has_animal && observation.step >= 73) {
                    if (impl_->flexible_worker < 0 ||
                        impl_->flexible_worker >= action.n_units)
                        impl_->flexible_worker = action.n_units - 1;
                    const int unit = impl_->flexible_worker;
                    const int x = observation.self().pos_x[unit];
                    const int y = observation.self().pos_y[unit];
                    if (observation.own.inv[unit][animal] > 0) {
                        if (x > 4) action.units[unit] = {kag::OP_WEST, 0, 1};
                        else if (x < 4) action.units[unit] = {kag::OP_EAST, 0, 1};
                        else if (y > 4) action.units[unit] = {kag::OP_NORTH, 0, 1};
                        else if (y < 4) action.units[unit] = {kag::OP_SOUTH, 0, 1};
                        else action.units[unit] = {
                            kag::OP_PLACE, static_cast<uint8_t>(animal), 1};
                    } else if (observation.own.shed[animal] > 0) {
                        action.units[unit] = {
                            kag::OP_PICKUP, static_cast<uint8_t>(animal), 1};
                    } else {
                        action.units[unit] = {};
                    }
                }
                action.finalize();
            }
            return;
        }
        if (impl_->active_shop != selected) {
            impl_->active_shop = selected;
            select(impl_->delegate, selected);
            std::visit([&](auto& agent) { agent.reset(impl_->init); },
                       impl_->delegate);
        }
    }
    std::visit([&](auto& agent) { agent.act(observation, budget, action); },
               impl_->delegate);
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::one_random_shop_dynamic_robust
