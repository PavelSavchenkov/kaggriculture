#include "agents/inhouse/fixed_weed_105492_robust/source/agent.hpp"

#include <cstdlib>
#include <memory>
#include <utility>

#include "agents/inhouse/fixed_weed_105492_robust/source/core.hpp"
#include "agents/inhouse/fixed_weed_105492_robust/source/donor_overlay.hpp"
#include "agents/inhouse/fixed_weed_105492_robust/source/reassignment_overlay.hpp"
#include "agents/inhouse/fixed_weed_105492_robust/source/residual_overlay.hpp"

namespace kag::agents::fixed_weed_105492_robust {

struct Agent::Impl {
    detail::CoreAgent core;
    detail::donor::DonorOverlay donor;
    detail::reassignment::Overlay reassignment;
    detail::residual::Overlay residual;

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

kag::agent::AgentInfo Agent::info() { return {"fixed_weed_105492_robust"}; }

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!impl_) std::abort();
    impl_->core.reset(init);
    impl_->donor.reset(init.player);
    impl_->reassignment.reset(init.player);
    impl_->residual.reset(init.player);
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
}

}
