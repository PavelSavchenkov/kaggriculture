#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "agents/inhouse/one_random_shop_dynamic_robust/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "agents/inhouse/one_random_shop_dynamic_robust/tests/one_random_shop_scenario.hpp"

namespace {

using Agent = kag::agents::one_random_shop_dynamic_robust::Agent;

void number_list(const std::vector<int>& values) {
    std::putchar('[');
    for (size_t index = 0; index < values.size(); ++index) {
        if (index) std::putchar(',');
        std::printf("%d", values[index]);
    }
    std::putchar(']');
}

}  // namespace

int main() {
    std::printf("{\"shops\":[");
    for (int shop = 0; shop < kag::N_SHOPS; ++shop) {
        kag::Config config;
        config.seed = 1;
        config.weed_chance = 0;
        kag::Sim sim(config);
        Agent agent;
        agent.reset(kag::agent::runtime::make_agent_init(sim, 0));
        kag::agent::DecisionBudget budget;
        kag::Action pass;
        pass.clear();
        pass.finalize();
        std::vector<int> offsets;
        std::vector<int> tape;
        while (!sim.st.done) {
            kag::Action action;
            agent.act(kag::agent::runtime::make_observation(sim, 0), budget,
                      action);
            if (!action.metadata_ready ||
                action.n_units != sim.st.farms[0].n_units ||
                action.n_orders < 0 || action.n_orders > sim.cfg.max_orders)
                std::abort();
            offsets.push_back(static_cast<int>(tape.size()));
            tape.push_back(action.n_units);
            tape.push_back(action.n_orders);
            for (int unit = 0; unit < action.n_units; ++unit) {
                tape.push_back(action.units[unit].op);
                tape.push_back(action.units[unit].arg);
                tape.push_back(action.units[unit].n);
            }
            for (int order = 0; order < action.n_orders; ++order) {
                tape.push_back(action.orders[order].op);
                tape.push_back(action.orders[order].item);
                tape.push_back(action.orders[order].n);
            }
            one_random_shop_scenario::step_forced(sim, action, pass, shop);
        }
        offsets.push_back(static_cast<int>(tape.size()));
        if (offsets.size() != 720) std::abort();
        if (shop) std::putchar(',');
        std::printf("{\"shop\":%d,\"cash\":%lld,\"offsets\":", shop,
                    static_cast<long long>(sim.st.farms[0].money));
        number_list(offsets);
        std::printf(",\"tape\":");
        number_list(tape);
        std::putchar('}');
    }
    std::printf("]}\n");
}
