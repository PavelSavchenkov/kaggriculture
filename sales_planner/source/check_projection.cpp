#include "case.hpp"
#include "project_stock.hpp"
#include "agents/common/runtime/observation_builder.hpp"

using namespace sales_planner;
int main(int argc, char** argv) {
    require(argc > 1, "usage: check_projection case.calendar [...]");
    int checked = 0, unsupported = 0;
    for (int i = 1; i < argc; ++i) {
        const auto c = read_case(argv[i]); kag::Sim sim(c.config);
        auto financial = c.initial.financial; auto resources = c.initial.resources;
        for (int t = 0; t < int(c.turns.size()); ++t) {
            const auto& turn = c.turns[t]; financial.n_shops = turn.n_shops; financial.shops = turn.shops;
            for (int p = 0; p < 2; ++p) {
                Account projected;
                const auto obs = kag::agent::runtime::make_observation(sim, p);
                const bool supported = project_stock(obs, turn.original_actions[p], projected);
                apply(financial.accounts[p], resources[p], turn.calendar[p].before_market);
                if (supported) {
                    if (projected.stock != financial.accounts[p].stock || projected.total != financial.accounts[p].total) {
                        std::fprintf(stderr, "projection mismatch episode=%llu turn=%d player=%d\n", (unsigned long long)c.episode, t, p);
                        return 1;
                    }
                    ++checked;
                } else ++unsupported;
            }
            trade(financial, turn.original_orders); consume(financial);
            for (int p = 0; p < 2; ++p) apply(financial.accounts[p], resources[p], turn.calendar[p].after_market);
            advance(financial);
            sim.step(turn.original_actions[0], turn.original_actions[1]);
        }
    }
    std::printf("{\"checked\":%d,\"unsupported_preserve_parent\":%d,\"status\":\"pass\"}\n", checked, unsupported);
}
