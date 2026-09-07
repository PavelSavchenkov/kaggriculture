#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/agent.hpp"

#include <cstdlib>
#include <memory>
#include <utility>

#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/core.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/donor_overlay.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/guard.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/reassignment_overlay.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/remap.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/residual_overlay.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/terminal_closure.hpp"

namespace kag::agents::pizza_125556_robust {

namespace {

pizza::Genome pizza_genome() {
    pizza::Genome genome;
    genome.cows = 7;
    genome.sheep = 1;
    genome.animal_order = 2;
    genome.tomato_mode = 0;
    genome.market_mode = 2;
    genome.sale_period = 4;
    genome.milk_floor = 160;
    genome.tomato_floor = 60;
    genome.shed_pressure = 99;
    genome.late_cow_additions = 2;
    genome.cow_group_override = -1;
    return genome;
}

}  // namespace

struct Agent::Impl {
    detail::CoreAgent core;
    detail::donor::DonorOverlay donor;
    detail::reassignment::Overlay reassignment;
    detail::residual::Overlay residual;
    pizza::RemapPolicy remap{pizza_genome()};

    Impl() {
        detail::RepairParameters repair;
        repair.cleanup_lookahead_days = 4;
        repair.cleanup_lookahead_kind_mask = uint32_t{1} << kag::WHEAT;
        repair.liquidity_sales = true;
        repair.liquidity_opportunity_selector = true;
        repair.underfilled_sale_substitution = true;
        core.set_parameters(repair);
        donor.set_parameters({true, uint32_t{1} << 19,
            (uint32_t{1} << kag::WHEAT) | (uint32_t{1} << kag::MELON),
            0, false, 5});
        reassignment.set_parameters({true,
            (uint32_t{1} << 11) | (uint32_t{1} << 17) | (uint32_t{1} << 19),
            0});
        residual.set_parameters({uint32_t{1} << 17, uint16_t{1} << 3, true});
    }
};

Agent::Agent() : impl_(std::make_unique<Impl>()) {}
Agent::~Agent() = default;
Agent::Agent(Agent&&) noexcept = default;
Agent& Agent::operator=(Agent&&) noexcept = default;

kag::agent::AgentInfo Agent::info() { return {"pizza_125556_robust"}; }

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!impl_) std::abort();
    impl_->core.reset(init);
    impl_->donor.reset(init.player);
    impl_->reassignment.reset(init.player);
    impl_->residual.reset(init.player);
    impl_->remap.reset();
}

void Agent::act(
    const kag::agent::AgentObservation& observation,
    const kag::agent::DecisionBudget& budget,
    kag::Action& action
) {
    if (!impl_) std::abort();
    impl_->core.act(observation, budget, action);
    impl_->donor.modify(observation, action);
    impl_->reassignment.modify(observation, action);
    impl_->residual.modify(observation, action);
    impl_->remap.modify(observation, action);
    pizza::close_terminal_milk(observation, action);
    pizza::guard_observation(observation, action);
}

}
