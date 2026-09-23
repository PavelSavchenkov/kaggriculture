#include "source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

uint64_t mix(uint64_t hash, const kag::Action& action) {
    auto add = [&hash](int value) { hash = (hash ^ static_cast<uint32_t>(value)) * 1099511628211ULL; };
    add(action.n_units); add(action.n_orders);
    for (int i = 0; i < action.n_units; ++i) {
        add(action.units[i].op); add(action.units[i].arg); add(action.units[i].n);
    }
    for (int i = 0; i < action.n_orders; ++i) {
        add(action.orders[i].op); add(action.orders[i].item); add(action.orders[i].n);
    }
    return hash;
}

uint64_t run(uint64_t seed, int seat) {
    kag::Config config;
    config.seed = seed;
    kag::Sim sim(config);
    kag::agents::agent_sep23::Agent agent;
    agent.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    kag::Action pass;
    pass.clear();
    pass.finalize();
    uint64_t hash = 1469598103934665603ULL;
    int turns = 0;
    while (!sim.st.done) {
        kag::Action action;
        agent.act(kag::agent::runtime::make_observation(sim, seat), budget, action);
        if (!action.metadata_ready || action.n_units != sim.st.farms[seat].n_units ||
            action.n_orders < 0 || action.n_orders > sim.cfg.max_orders)
            throw std::runtime_error("invalid production action");
        hash = mix(hash, action);
        if (seat == 0) sim.step(action, pass); else sim.step(pass, action);
        ++turns;
    }
    agent.finish(kag::agent::runtime::make_observation(sim, seat));
    if (turns != config.episode_steps - 1) throw std::runtime_error("incomplete game");
    for (const auto& report : agent.reports())
        if (report.day >= 0 && report.max_step_attempts > 256)
            throw std::runtime_error("compiler exceeded node budget");
    return hash;
}

}

int main() {
    for (int seat : {0, 1}) {
        const auto first = run(923001, seat);
        const auto second = run(923001, seat);
        if (first != second) throw std::runtime_error("repeat run was not deterministic");
    }
    std::cout << "4 complete games: valid actions, 719 turns, deterministic reset, <=256 attempts\n";
}
