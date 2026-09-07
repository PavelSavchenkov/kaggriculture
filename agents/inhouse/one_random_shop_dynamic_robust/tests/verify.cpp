#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "agents/inhouse/one_random_shop_dynamic_robust/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/tests/one_random_shop_scenario.hpp"

namespace {

using Agent = kag::agents::one_random_shop_dynamic_robust::Agent;

int run(uint64_t seed, int seat, int shop) {
    kag::Config config;
    config.seed = seed;
    config.weed_chance = 0.005;
    kag::Sim sim(config);
    Agent agent;
    agent.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    kag::Action pass;
    pass.clear();
    pass.finalize();
    while (!sim.st.done) {
        kag::Action action;
        agent.act(kag::agent::runtime::make_observation(sim, seat), budget,
                  action);
        if (!action.metadata_ready ||
            action.n_units != sim.st.farms[seat].n_units ||
            action.n_orders < 0 || action.n_orders > sim.cfg.max_orders)
            std::abort();
        if (seat == 0)
            one_random_shop_scenario::step_forced(sim, action, pass, shop);
        else
            one_random_shop_scenario::step_forced(sim, pass, action, shop);
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (sim.st.farms[seat].discarded[item] != 0) std::abort();
    return static_cast<int>(sim.st.farms[seat].money);
}

}  // namespace

int main() {
    std::array<int64_t, kag::N_SHOPS> sums{};
    for (uint64_t seed = 61000001; seed < 61000065; ++seed)
        for (int seat = 0; seat < 2; ++seat)
            for (int shop = 0; shop < kag::N_SHOPS; ++shop)
                sums[shop] += run(seed, seat, shop);
    for (int shop = 0; shop < kag::N_SHOPS; ++shop)
        std::printf("shop=%d mean=%.6f\n", shop,
                    static_cast<double>(sums[shop]) / 128.0);
}
