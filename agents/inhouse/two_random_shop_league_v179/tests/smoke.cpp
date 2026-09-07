#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "agents/common/runtime/observation_builder.hpp"
#include "agents/inhouse/two_random_shop_league_v179/source/agent.hpp"

namespace {

uint64_t hash_action(uint64_t hash, const kag::Action& action) {
    const auto add = [&](uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    add(action.n_units);
    add(action.n_orders);
    for (int unit = 0; unit < action.n_units; ++unit) {
        add(action.units[unit].op);
        add(action.units[unit].arg);
        add(action.units[unit].n);
    }
    for (int order = 0; order < action.n_orders; ++order) {
        add(action.orders[order].op);
        add(action.orders[order].item);
        add(action.orders[order].n);
    }
    return hash;
}

uint64_t run(uint64_t seed, int seat, int first, int second,
             kag::agents::two_random_shop_league_v179::Agent& agent) {
    kag::Config config;
    config.seed = seed;
    config.weed_chance = 0.005;
    kag::Sim sim(config);
    agent.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    uint64_t hash = 1469598103934665603ULL;
    while (!sim.st.done) {
        kag::Action action;
        agent.act(kag::agent::runtime::make_observation(sim, seat), budget,
                  action);
        if (!action.metadata_ready || action.n_units < 0 ||
            action.n_units > kag::MAX_UNITS || action.n_orders < 0 ||
            action.n_orders > 10)
            std::abort();
        hash = hash_action(hash, action);
        kag::Action pass;
        pass.clear();
        pass.finalize();
        if (sim.st.step == 143) sim.cfg.shop_unlock_interval = 1000;
        sim.step(seat ? pass : action, seat ? action : pass);
        if (sim.st.n_shops >= 1)
            sim.st.shops[0] = static_cast<uint8_t>(first);
        if (sim.st.n_shops >= 2)
            sim.st.shops[1] = static_cast<uint8_t>(second);
    }
    hash ^= static_cast<uint64_t>(sim.st.farms[seat].money);
    hash *= 1099511628211ULL;
    return hash;
}

}  // namespace

int main() {
    kag::agents::two_random_shop_league_v179::Agent reused;
    uint64_t combined = 1469598103934665603ULL;
    for (uint64_t seed = 892000001; seed < 892000003; ++seed)
        for (int seat = 0; seat < 2; ++seat)
            for (int first = 0; first < kag::N_SHOPS; ++first)
                for (int second = 0; second < kag::N_SHOPS; ++second) {
                    const uint64_t first_hash =
                        run(seed, seat, first, second, reused);
                    kag::agents::two_random_shop_league_v179::Agent fresh;
                    const uint64_t second_hash =
                        run(seed, seat, first, second, fresh);
                    if (first_hash != second_hash) std::abort();
                    combined ^= first_hash;
                    combined *= 1099511628211ULL;
                }
    std::printf("PASS reset_determinism hash=%llu\n",
                static_cast<unsigned long long>(combined));
}
