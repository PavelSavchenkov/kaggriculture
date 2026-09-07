#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/agent.hpp"

#include <cstdlib>
#include <memory>
#include <utility>

#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/core.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/donor_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/reassignment_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/residual_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/residual_route_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/target8_overlay.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/brunch_114643_robust/cleanup_route_overlay.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::brunch_114643_robust {

struct Agent::Impl {
    detail::CoreAgent core;
    detail::donor::DonorOverlay donor;
    detail::reassignment::Overlay reassignment;
    detail::residual::Overlay residual;
    detail::residual_route::Overlay residual_route;
    detail::target8::Overlay target8;
    detail::cleanup::Overlay cleanup_route;

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
            (uint32_t{1} << 11) | (uint32_t{1} << 17) |
                (uint32_t{1} << 19),
            0});
        residual.set_parameters({uint32_t{1} << 17, uint16_t{24314}, false});
        residual_route.set_parameters({uint32_t{1} << 17,
            static_cast<uint16_t>((uint16_t{1} << 1) |
                                  (uint16_t{1} << 6)),
            true, true, true, false});
        cleanup_route.set_enabled(true);
        cleanup_route.set_route265_enabled(true);
        cleanup_route.set_route121_enabled(true);
        cleanup_route.set_route241_enabled(true);
        cleanup_route.set_overflow599_enabled(true);
        target8.set_mode(detail::target8::Mode::route_safe);
    }
};

Agent::Agent() : impl_(std::make_unique<Impl>()) {}
Agent::~Agent() = default;
Agent::Agent(Agent&&) noexcept = default;
Agent& Agent::operator=(Agent&&) noexcept = default;

kag::agent::AgentInfo Agent::info() { return {"brunch_114643_robust"}; }

void Agent::reset(const kag::agent::AgentInit& init) {
    if (!impl_) std::abort();
    impl_->core.reset(init);
    impl_->donor.reset(init.player);
    impl_->reassignment.reset(init.player);
    impl_->residual.reset(init.player);
    impl_->residual_route.reset(init.player);
    impl_->target8.reset(init.player);
    impl_->cleanup_route.reset(init.player);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    if (!impl_) std::abort();
    impl_->core.act(observation, budget, action);
    impl_->donor.modify(observation, action);
    impl_->reassignment.modify(observation, action);
    impl_->residual.modify(observation, action);
    impl_->residual_route.modify(observation, action);
    impl_->target8.modify(observation, action);
    impl_->cleanup_route.modify(observation, action);
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::brunch_114643_robust
