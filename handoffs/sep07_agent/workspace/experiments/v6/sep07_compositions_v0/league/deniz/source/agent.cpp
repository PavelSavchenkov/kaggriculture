#include "agent.hpp"

#include <algorithm>

#include "observation_sim.hpp"

namespace four_shop_foundry::public_adapted::deniz_v111_safe {
namespace {

kag::Action normalize_noops(kag::Action action) {
    for (int unit = 0; unit < action.n_units; ++unit)
        if (action.units[unit].op == kag::OP_PASS) action.units[unit] = {};
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_NONE ||
            (order.op != kag::M_HIRE && order.op != kag::M_BUY_LAND &&
             order.n <= 0))
            action.orders[index] = {};
    }
    action.finalize();
    return action;
}

kag::Action exact_fast_sanitize(const kag::Sim& sim, int player,
                                const kag::Action& requested) {
    const kag::Farm& farm = sim.st.farms[player];
    if (!requested.metadata_ready || requested.n_units != farm.n_units ||
        requested.n_orders < 0 || requested.n_orders > sim.cfg.max_orders)
        return sim.sanitize_solo_action(player, requested);

    const kag::Sim::SoloActionOutcome outcome =
        sim.diagnose_solo_action(player, requested, true);
    if (outcome.requested_unit_actions == outcome.successful_unit_actions &&
        outcome.requested_order_units == outcome.successful_order_units)
        return normalize_noops(requested);

    kag::Action result;
    result.clear();
    result.n_units = farm.n_units;
    std::fill_n(result.units, result.n_units, kag::UnitAction{});

    kag::Action prefix = requested;
    std::fill_n(prefix.units, prefix.n_units, kag::UnitAction{});
    prefix.n_orders = 0;
    int prior_successes = 0;
    for (int unit = 0; unit < prefix.n_units; ++unit) {
        prefix.units[unit] = requested.units[unit];
        const kag::Sim::SoloActionOutcome current =
            sim.diagnose_solo_action(player, prefix, true);
        const int delta = current.successful_unit_actions - prior_successes;
        if (delta < 0 || delta > 1)
            return sim.sanitize_solo_action(player, requested);
        if (delta == 1) result.units[unit] = requested.units[unit];
        prior_successes = current.successful_unit_actions;
    }
    if (prior_successes != outcome.successful_unit_actions)
        return sim.sanitize_solo_action(player, requested);
    result.finalize();

    prefix = requested;
    prefix.n_orders = 0;
    result.n_orders = requested.n_orders;
    std::fill_n(result.orders, result.n_orders, kag::Order{});
    prior_successes = 0;
    for (int index = 0; index < result.n_orders; ++index) {
        prefix.n_orders = index + 1;
        const kag::Sim::SoloActionOutcome current =
            sim.diagnose_solo_action(player, prefix, true);
        const int accepted = current.successful_order_units - prior_successes;
        const kag::Order& order = requested.orders[index];
        const int maximum = order.op == kag::M_HIRE ||
            order.op == kag::M_BUY_LAND ? 1 : std::max(0, order.n);
        if (accepted < 0 || accepted > maximum)
            return sim.sanitize_solo_action(player, requested);
        if (accepted > 0) {
            if (order.op == kag::M_HIRE || order.op == kag::M_BUY_LAND)
                result.orders[index] = order;
            else
                result.orders[index] = {order.op, order.item, accepted};
        }
        prior_successes = current.successful_order_units;
    }
    if (prior_successes != outcome.successful_order_units)
        return sim.sanitize_solo_action(player, requested);
    return result;
}

}

kag::agent::AgentInfo Agent::info() { return {"public_deniz_v111_safe"}; }

void Agent::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    source_.reset(init);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget& budget,
                kag::Action& action) {
    kag::Action requested;
    source_.act(observation, budget, requested);
    const kag::Sim sim = observation_sim::make_sim(config_, observation);
    action = exact_fast_sanitize(sim, observation.player, requested);
}

}
