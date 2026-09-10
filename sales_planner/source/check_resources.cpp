#include "case.hpp"
#include "project_resources.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <chrono>

using namespace sales_planner;
int main(int argc, char** argv) {
    require(argc > 1, "usage: check_resources case.calendar [...]");
    int checked = 0; double seconds = 0;
    for (int i = 1; i < argc; ++i) {
        const auto c = read_case(argv[i]); kag::Sim sim(c.config);
        auto financial = c.initial.financial; auto resources = c.initial.resources;
        for (int t = 0; t < int(c.turns.size()); ++t) {
            const auto& turn = c.turns[t]; financial.n_shops = turn.n_shops; financial.shops = turn.shops;
            for (int p = 0; p < 2; ++p) {
                Account projected; Resources carried;
                const auto obs = kag::agent::runtime::make_observation(sim, p);
                const auto started = std::chrono::steady_clock::now();
                require(project_resources(obs, turn.original_actions[p], projected, carried,
                                          c.config.shed_capacity, c.config.turns_per_day), "unsupported resource projection");
                seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
                apply(financial.accounts[p], resources[p], turn.calendar[p].before_market);
                bool equal = projected.stock == financial.accounts[p].stock && projected.total == financial.accounts[p].total &&
                             projected.seeds == financial.accounts[p].seeds && carried.buffers == resources[p].buffers &&
                             carried.key_count == resources[p].key_count;
                for (int u = 0; u < kag::MAX_UNITS; ++u)
                    for (int k = 0; k < carried.key_count[u]; ++k)
                        if (carried.keys[u][k] != resources[p].keys[u][k]) equal = false;
                if (!equal) {
                    std::fprintf(stderr, "resource projection mismatch episode=%llu turn=%d player=%d\n", (unsigned long long)c.episode, t, p);
                    return 1;
                }
                ++checked;
            }
            trade(financial, turn.original_orders); consume(financial);
            for (int p = 0; p < 2; ++p) apply(financial.accounts[p], resources[p], turn.calendar[p].after_market);
            advance(financial); sim.step(turn.original_actions[0], turn.original_actions[1]);
        }
    }
    std::printf("{\"checked\":%d,\"seconds\":%.6f,\"microseconds_per_projection\":%.6f,\"status\":\"pass\"}\n", checked, seconds, seconds * 1e6 / checked);
}
