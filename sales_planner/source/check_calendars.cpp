#include "case.hpp"
#include <chrono>

using namespace sales_planner;

static void compare(const MarketState& got, const std::array<Resources, 2>& resources,
                    const Truth& expected, uint64_t episode, int turn, const char* layer) {
    const auto& want = expected.financial;
    auto check = [&](bool ok, const char* field) {
        if (!ok) {
            std::fprintf(stderr, "FAIL episode=%llu turn=%d layer=%s field=%s cash=%.0f/%.0f expected=%.0f/%.0f\n",
                         (unsigned long long)episode, turn, layer, field,
                         got.accounts[0].cash, got.accounts[1].cash, want.accounts[0].cash, want.accounts[1].cash);
            std::exit(1);
        }
    };
    check(got.inventory == want.inventory, "market");
    for (int p = 0; p < 2; ++p) {
        const auto& a = got.accounts[p]; const auto& b = want.accounts[p];
        check(a.cash == b.cash, "cash");
        check(a.stock == b.stock && a.total == b.total, "stock");
        check(a.seeds == b.seeds, "seeds");
        check(a.units == b.units && a.hires == b.hires && a.quadrants == b.quadrants, "commitments");
        check(resources[p].buffers == expected.resources[p].buffers, "buffers");
        for (int n : resources[p].missing) check(n == 0, "missing input");
    }
}

int main(int argc, char** argv) {
    require(argc > 1, "usage: check_calendars case.calendar [...]");
    uint64_t checked = 0;
    double seconds = 0;
    for (int i = 1; i < argc; ++i) {
        const auto c = read_case(argv[i]);
        auto state = c.initial.financial; auto resources = c.initial.resources;
        kag::Sim sim(c.config);
        const auto rules = rules_for(c.config);
        const auto started = std::chrono::steady_clock::now();
        for (int t = 0; t < int(c.turns.size()); ++t) {
            const auto& turn = c.turns[t];
            state.n_shops = turn.n_shops; state.shops = turn.shops;
            for (int p = 0; p < 2; ++p)
                apply(state.accounts[p], resources[p], turn.calendar[p].before_market, rules.capacity);
            trade(state, turn.original_orders, rules); consume(state, rules);
            for (int p = 0; p < 2; ++p)
                apply(state.accounts[p], resources[p], turn.calendar[p].after_market, rules.capacity);
            advance(state, rules);
            compare(state, resources, turn.expected, c.episode, t, "financial");
            sim.step(turn.original_actions[0], turn.original_actions[1]);
            std::array<Resources, 2> engine_resources;
            for (int p = 0; p < 2; ++p)
                for (int u = 0; u < sim.st.farms[p].n_units; ++u)
                    for (int item = 0; item < kag::N_ITEMS; ++item)
                        engine_resources[p].buffers[u][item] = sim.st.farms[p].inv[u][item];
            compare(financial_state(sim.st), engine_resources, turn.expected, c.episode, t, "full_engine");
            ++checked;
        }
        seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::printf("{\"episode\":%llu,\"turns\":%zu,\"cash\":[%.0f,%.0f],\"status\":\"pass\"}\n",
                    (unsigned long long)c.episode, c.turns.size(), state.accounts[0].cash, state.accounts[1].cash);
    }
    std::fprintf(stderr, "checked=%llu comparison_seconds=%.6f\n", (unsigned long long)checked, seconds);
}
