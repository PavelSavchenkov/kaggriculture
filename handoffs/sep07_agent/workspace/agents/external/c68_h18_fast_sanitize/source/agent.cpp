#include "agent.hpp"

#include <array>
#include <cstdlib>
#include <utility>

#include "base/generated_actions.hpp"
#include "optimized_active_safety.hpp"

namespace four_shop_foundry::throughput::c68_h18_fast_sanitize {
namespace {

using league::public_unchanged::c68::generated::ORDERS;
using league::public_unchanged::c68::generated::STEPS;

constexpr std::array<kag::Order, 10> EXPECTED{{
    {kag::M_HIRE, kag::WHEAT, 0},
    {kag::M_HIRE, kag::WHEAT, 0},
    {kag::M_HIRE, kag::WHEAT, 0},
    {kag::M_HIRE, kag::WHEAT, 0},
    {kag::M_HIRE, kag::WHEAT, 0},
    {kag::M_BUY_ANIMAL, kag::COW, 1},
    {kag::M_BUY_ANIMAL, kag::SHEEP, 4},
    {kag::M_BUY_SEED, kag::WHEAT, 5},
    {kag::M_BUY_SEED, kag::MELON, 5},
    {kag::M_BUY_PRODUCT, kag::WHEAT, 5},
}};

constexpr bool same_order(const kag::Order& left, const kag::Order& right) {
    return left.op == right.op && left.item == right.item && left.n == right.n;
}

constexpr bool generated_opener_matches() {
    if (STEPS[0].order_offset != 0 || STEPS[0].n_orders != EXPECTED.size())
        return false;
    for (int index = 0; index < static_cast<int>(EXPECTED.size()); ++index)
        if (!same_order(ORDERS[index], EXPECTED[index])) return false;
    return true;
}

static_assert(generated_opener_matches());

void require_effective_h6_opener(const kag::Action& action) {
    if (action.n_orders != static_cast<int>(EXPECTED.size())) std::abort();
    for (int index = 0; index < action.n_orders; ++index)
        if (!same_order(action.orders[index], EXPECTED[index])) std::abort();
}

}

kag::agent::AgentInfo Agent::info() {
    return {"c68-h18-fast-sanitize"};
}

void Agent::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    source_.reset(init);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    source_.act(observation, budget, action);
    four_shop_foundry::optimized_active_safety::sanitize(
        config_, observation, action);
    if (observation.step != 0) return;
    require_effective_h6_opener(action);
    std::swap(action.orders[5], action.orders[9]);
    action.finalize();
}

}
