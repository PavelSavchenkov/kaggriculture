#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include "agents/common/runtime/observation_builder.hpp"
#include "agents/inhouse/fixed_weed_105492_robust/source/agent.hpp"

namespace {

using Agent = kag::agents::fixed_weed_105492_robust::Agent;

struct Episode {
    double cash = 0;
    int discarded = 0;
};

uint64_t parse_u64(const char* text) {
    char* end = nullptr;
    const uint64_t value = std::strtoull(text, &end, 10);
    if (!end || *end) std::abort();
    return value;
}

Episode run(uint64_t seed, int seat) {
    kag::Config config;
    config.seed = seed;
    config.shop_unlock_interval = 1000;
    config.weed_chance = 0.005;
    kag::Sim sim(config);
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
    Episode result;
    result.cash = sim.st.farms[seat].money;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        result.discarded += sim.st.farms[seat].discarded[item];
    return result;
}

double lower_cvar(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const double mass = 0.1 * values.size();
    const size_t whole = static_cast<size_t>(std::floor(mass));
    const double fraction = mass - whole;
    double sum = 0;
    for (size_t index = 0; index < whole; ++index) sum += values[index];
    if (fraction) sum += fraction * values[whole];
    return sum / mass;
}

}

int main(int argc, char** argv) {
    if (argc != 4) std::abort();
    const uint64_t seed_start = parse_u64(argv[1]);
    const int seeds = static_cast<int>(parse_u64(argv[2]));
    const int threads = std::clamp(static_cast<int>(parse_u64(argv[3])), 1, 32);
    std::vector<Episode> episodes(seeds * 2);
    std::atomic<int> next{0};
    std::vector<std::thread> workers;
    for (int thread = 0; thread < threads; ++thread)
        workers.emplace_back([&] {
            for (;;) {
                const int index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= static_cast<int>(episodes.size())) return;
                episodes[index] = run(seed_start + index / 2, index & 1);
            }
        });
    for (std::thread& worker : workers) worker.join();

    std::vector<double> cash;
    cash.reserve(episodes.size());
    double sum = 0;
    long discarded = 0;
    for (const Episode& episode : episodes) {
        cash.push_back(episode.cash);
        sum += episode.cash;
        discarded += episode.discarded;
    }
    const double mean = sum / episodes.size();
    const double cvar = lower_cvar(cash);
    const auto [minimum, maximum] = std::minmax_element(cash.begin(), cash.end());
    std::printf("seeds=%d start=%llu mean=%.6f cvar10=%.6f J=%.6f "
                "min=%.0f max=%.0f discards=%ld\n",
        seeds, static_cast<unsigned long long>(seed_start), mean, cvar,
        0.8 * mean + 0.2 * cvar, *minimum, *maximum, discarded);
}
