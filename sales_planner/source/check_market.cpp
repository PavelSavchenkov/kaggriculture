#include "calendar.hpp"
#include <chrono>
#include <cstdio>
#include <random>

using namespace sales_planner;

static void check(bool ok, const char* what, int game, int turn) {
    if (!ok) { std::fprintf(stderr, "FAIL %s game=%d turn=%d\n", what, game, turn); std::exit(1); }
}

static void compare(const MarketState& s, const kag::State& ref, int game, int turn) {
    const auto expected = financial_state(ref);
    check(s.turn == expected.turn, "turn", game, turn);
    check(s.inventory == expected.inventory, "market inventory", game, turn);
    for (int p = 0; p < 2; ++p) {
        const auto& a = s.accounts[p]; const auto& b = expected.accounts[p];
        check(a.cash == b.cash, "cash", game, turn);
        check(a.total == b.total && a.stock == b.stock, "stock", game, turn);
        check(a.seeds == b.seeds, "seeds", game, turn);
        check(a.units == b.units && a.hires == b.hires, "hires", game, turn);
        check(a.quadrants == b.quadrants, "land", game, turn);
    }
}

int main(int argc, char** argv) {
    const int games = argc > 1 ? std::atoi(argv[1]) : 256;
    std::mt19937 rng(7091801);
    double kernel_seconds = 0, engine_seconds = 0;
    uint64_t transitions = 0, accepted = 0;
    for (int game = 0; game < games; ++game) {
        kag::Config cfg; cfg.seed = rng(); cfg.weed_chance = 0;
        cfg.max_orders = 1 + rng() % 16;
        cfg.shed_capacity = 20 + rng() % 81;
        cfg.hire_mult = 1 + rng() % 3;
        kag::Sim sim(cfg);
        MarketRules rules{cfg.shed_capacity, cfg.max_orders, cfg.hire_mult};
        for (int p = 0; p < 2; ++p) sim.st.farms[p].money = rng() % 20000;
        for (int i = 0; i < kag::N_PRODUCTS; ++i) {
            sim.st.market.inventory[i] = 9200 + rng() % 1800;
            sim.st.market.prices[i] = kag::market_price(i, sim.st.market.inventory[i]);
        }
        auto state = financial_state(sim.st);
        for (int turn = 0; turn < terminal_turn; ++turn) {
            // Resource injections are independent of trade. They exercise full
            // and empty sheds, retained stock, and resumed post-worker states.
            std::array<Orders, 2> orders;
            kag::Action actions[2];
            for (int p = 0; p < 2; ++p) {
                auto& f = sim.st.farms[p]; auto& a = state.accounts[p];
                for (int i = 0; i < kag::N_ITEMS; ++i) {
                    const int delta = std::min(int(rng() % 7), cfg.shed_capacity - a.total);
                    if (rng() % 8 == 0) {
                        f.shed[i] += delta; f.shed_total += delta;
                        a.stock[i] += delta; a.total += delta;
                    }
                }
                actions[p].clear(); actions[p].n_units = f.n_units;
                for (int u = 0; u < f.n_units; ++u) actions[p].units[u] = {};
                orders[p].count = rng() % 17;
                for (int slot = 0; slot < orders[p].count; ++slot) {
                    const uint8_t op = rng() % 8, item = rng() % 14;
                    const int quantity = int(rng() % 35) - 2;
                    orders[p].values[slot] = {op, item, quantity};
                    actions[p].orders[slot] = {op, item, quantity};
                }
                actions[p].n_orders = orders[p].count; actions[p].finalize();
            }
            const double old_receipts[2] = {sim.st.farms[0].sell_revenue, sim.st.farms[1].sell_revenue};
            const double old_spending[2] = {sim.st.farms[0].total_spend, sim.st.farms[1].total_spend};
            const auto a = std::chrono::steady_clock::now();
            const auto result = trade(state, orders, rules);
            consume(state, rules); advance(state, rules);
            const auto b = std::chrono::steady_clock::now();
            sim.step(actions[0], actions[1]);
            const auto c = std::chrono::steady_clock::now();
            kernel_seconds += std::chrono::duration<double>(b-a).count();
            engine_seconds += std::chrono::duration<double>(c-b).count();
            compare(state, sim.st, game, turn);
            for (int p = 0; p < 2; ++p) {
                check(result.receipts[p] == sim.st.farms[p].sell_revenue - old_receipts[p], "receipts", game, turn);
                check(result.spending[p] == sim.st.farms[p].total_spend - old_spending[p], "spending", game, turn);
                for (int n : result.accepted[p]) accepted += n;
            }
            state.n_shops = sim.st.n_shops;
            std::copy_n(sim.st.shops, state.n_shops, state.shops.begin());
            ++transitions;
        }
        check(sim.st.done, "terminal", game, terminal_turn);
    }
    // Resource chronology: a deposit fills remaining capacity; PLACE retains
    // the rest, DROP discards it, and missing inputs stay visible.
    Account account; Resources resources;
    account.stock[kag::WHEAT] = account.total = 98;
    const std::array<ResourceEvent, 5> events{{
        {Flow::produce, kag::MILK, 0, 6}, {Flow::deposit, kag::MILK, 0, 6},
        {Flow::withdraw, kag::FERTILIZER, 1, 2}, {Flow::drop, kag::MILK, 0, 4},
        {Flow::use_seed, kag::WHEAT, 0, 1}}};
    apply(account, resources, std::span<const ResourceEvent>(events));
    check(account.total == 100 && account.stock[kag::MILK] == 2, "deposit cap", -1, 0);
    check(resources.discarded[kag::MILK] == 4 && resources.buffers[0][kag::MILK] == 0, "drop cap", -1, 0);
    check(resources.missing[kag::FERTILIZER] == 2 && resources.missing[kag::WHEAT] == 1, "input gaps", -1, 0);
    std::printf("{\"games\":%d,\"transitions\":%llu,\"accepted_units\":%llu,\"kernel_seconds\":%.6f,\"engine_seconds\":%.6f,\"kernel_us_per_turn\":%.6f,\"status\":\"pass\"}\n",
                games, (unsigned long long)transitions, (unsigned long long)accepted,
                kernel_seconds, engine_seconds, kernel_seconds * 1e6 / transitions);
}
