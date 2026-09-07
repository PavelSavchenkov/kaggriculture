#include <cstdio>
#include <cstdlib>

#include "agents/common/runtime/observation_builder.hpp"
#include "agents/inhouse/fixed_weed_105492_robust/source/agent.hpp"

namespace {

using Agent = kag::agents::fixed_weed_105492_robust::Agent;
static_assert(kag::agent::LocalAgent<Agent>);

kag::Config config_for(uint64_t seed, double weed_chance) {
    kag::Config config;
    config.seed = seed;
    config.shop_unlock_interval = 1000;
    config.weed_chance = weed_chance;
    return config;
}

double run_pass(uint64_t seed, int seat, double weed_chance) {
    kag::Sim sim(config_for(seed, weed_chance));
    Agent agent;
    agent.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    kag::Action action;
    kag::Action pass;
    pass.clear();
    pass.finalize();
    while (!sim.st.done) {
        agent.act(kag::agent::runtime::make_observation(sim, seat), budget, action);
        if (!action.metadata_ready || action.n_units != sim.st.farms[seat].n_units ||
            action.n_orders < 0 || action.n_orders > sim.cfg.max_orders)
            std::abort();
        if (seat == 0) sim.step(action, pass);
        else sim.step(pass, action);
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (sim.st.farms[seat].discarded[item]) std::abort();
    return sim.st.farms[seat].money;
}

void self_play() {
    kag::Sim sim(config_for(271828, 0.005));
    Agent agents[2];
    kag::Action actions[2];
    kag::agent::DecisionBudget budget;
    for (int seat = 0; seat < 2; ++seat)
        agents[seat].reset(kag::agent::runtime::make_agent_init(sim, seat));
    while (!sim.st.done) {
        for (int seat = 0; seat < 2; ++seat) {
            agents[seat].act(kag::agent::runtime::make_observation(sim, seat), budget,
                actions[seat]);
            if (!actions[seat].metadata_ready ||
                actions[seat].n_units != sim.st.farms[seat].n_units ||
                actions[seat].n_orders > sim.cfg.max_orders)
                std::abort();
        }
        sim.step(actions[0], actions[1]);
    }
}

}

int main() {
    const double fixed0 = run_pass(1, 0, 0);
    const double fixed1 = run_pass(1, 1, 0);
    if (fixed0 != 105492 || fixed1 != 105492) std::abort();
    const double overflow = run_pass(16060896, 0, 0.005);
    if (overflow != 104894) std::abort();
    run_pass(9173, 0, 0.005);
    run_pass(1009173, 1, 0.005);
    self_play();
    std::printf("fixed_cash=%.0f/%.0f overflow_seed=%.0f random_smoke=ok "
                "contracts=ok discards=0 self_play=ok\n",
        fixed0, fixed1, overflow);
}
