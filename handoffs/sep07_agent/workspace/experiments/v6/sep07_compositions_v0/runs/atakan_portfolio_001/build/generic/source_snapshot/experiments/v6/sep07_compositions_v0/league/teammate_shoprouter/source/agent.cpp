#include "agent.hpp"
#include "upstream/policy.hpp"

namespace compositions::teammate_shoprouter {

void Agent::reset(const kag::agent::AgentInit& init) {
    config_ = kag::Config{};
    config_.episode_steps = init.config.episode_steps;
    config_.max_orders = init.config.max_orders;
    config_.hire_mult = init.config.hire_mult;
    route_ = 0;
}

void Agent::act(const kag::agent::AgentObservation& o,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    kag::State state{};
    state.step = o.step;
    state.day = o.day;
    state.hour = o.hour;
    state.market = o.market;
    state.n_shops = o.n_shops;
    std::copy_n(o.shops, o.n_shops, state.shops);
    for (int seat = 0; seat < 2; ++seat) {
        auto& f = state.farms[seat];
        const auto& p = o.farms[seat];
        f.money = p.money;
        f.n_units = p.n_units;
        f.n_quadrants = p.n_quadrants;
        f.hires_today = p.hires_today;
        for (int y = 0; y < kag::BOARD; ++y)
            for (int x = 0; x < kag::BOARD; ++x) f.tiles[y][x] = p.tiles[y][x];
        std::copy_n(p.pos_x, p.n_units, f.pos_x);
        std::copy_n(p.pos_y, p.n_units, f.pos_y);
    }
    auto& own = state.farms[o.player];
    own.shed_total = o.own.shed_total;
    std::copy_n(o.own.shed, kag::N_ITEMS, own.shed);
    for (int u = 0; u < own.n_units; ++u)
        std::copy_n(o.own.inv[u], kag::N_ITEMS, own.inv[u]);
    Context context;
    context.selected_route = route_;
    action = context.act(state, config_, o.player);
    route_ = context.selected_route;

    // Literal translation of the teammate's additive sigmoid selling head.
    constexpr double bias[8] = {-0.775, -1.163, 1.852, 1.362, -1.676, -1.504, 0.19, -0.948};
    bool already[kag::N_PRODUCTS]{};
    for (int i = 0; i < action.n_orders; ++i)
        if (action.orders[i].op == kag::M_SELL && action.orders[i].item < kag::N_PRODUCTS)
            already[action.orders[i].item] = true;
    const double margin = std::clamp((o.self().money - o.opponent().money) / 50000.0, -2.0, 2.0);
    for (int item = 1; item < kag::N_PRODUCTS && action.n_orders < config_.max_orders; ++item) {
        const int q = o.own.shed[item];
        if (already[item] || q <= 0 || o.market.prices[item] < 3) continue;
        double z = bias[item - 1] + 2.001 * (std::min(q, 100) / 50.0);
        z += -0.131 * ((o.market.inventory[item] - 10000) / 10000.0);
        z += -0.104 * (o.step / 720.0) - 7.566 * margin - 0.574;
        const int n = static_cast<int>(std::nearbyint(q / (1.0 + std::exp(-std::clamp(z, -30.0, 30.0)))));
        if (n > 0) action.orders[action.n_orders++] = {kag::M_SELL, static_cast<uint8_t>(item), n};
    }
    for (int u = action.n_units; u < own.n_units; ++u) action.units[u] = {};
    action.n_units = own.n_units;
    action.finalize();
}
}
