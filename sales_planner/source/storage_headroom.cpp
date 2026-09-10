#include "case.hpp"
#include <chrono>
#include <numeric>

using namespace sales_planner;

struct Outcome {
    Truth state;
    std::array<int, 2> commitment_errors{};
};
struct Edit { int turn = -1, seat = 0, item = 0, quantity = 0; };
struct Loss { int turn, phase, quantity; Items discarded; };

static int sum(const Items& a) { return std::accumulate(a.begin(), a.end(), 0); }

static void step(const Case& c, int t, Outcome& out, Edit edit = {}) {
    const auto& turn = c.turns[t];
    auto& s = out.state.financial;
    const auto rules = rules_for(c.config);
    s.n_shops = turn.n_shops; s.shops = turn.shops;
    for (int p = 0; p < 2; ++p)
        apply(s.accounts[p], out.state.resources[p], turn.calendar[p].before_market, rules.capacity);
    auto orders = turn.original_orders;
    if (edit.turn == t) {
        auto& own = orders[edit.seat];
        require(own.count < rules.max_orders, "no room for diagnostic sale");
        own.values[own.count++] = {kag::M_SELL, uint8_t(edit.item), edit.quantity};
    }
    const auto traded = trade(s, orders, rules);
    for (int p = 0; p < 2; ++p) {
        int required[3]{}, accepted[3]{};
        for (int k = 0; k < turn.calendar[p].commitments.count; ++k) {
            const auto op = turn.calendar[p].commitments.values[k].op;
            if (op == kag::M_HIRE || op == kag::M_BUY_LAND) ++required[op];
        }
        for (int k = 0; k < orders[p].count; ++k) {
            const auto op = orders[p].values[k].op;
            if (op == kag::M_HIRE || op == kag::M_BUY_LAND) accepted[op] += traded.accepted[p][k];
        }
        out.commitment_errors[p] += std::abs(required[kag::M_HIRE] - accepted[kag::M_HIRE]) +
                                   std::abs(required[kag::M_BUY_LAND] - accepted[kag::M_BUY_LAND]);
    }
    consume(s, rules);
    for (int p = 0; p < 2; ++p)
        apply(s.accounts[p], out.state.resources[p], turn.calendar[p].after_market, rules.capacity);
    advance(s, rules);
}

static bool preserves(const Outcome& trial, const Outcome& original, int seat) {
    for (int p = 0; p < 2; ++p)
        if (sum(trial.state.resources[p].missing) || trial.commitment_errors[p]) return false;
    const auto& a = trial.state.financial.accounts[seat];
    const auto& b = original.state.financial.accounts[seat];
    for (int i = 0; i < kag::N_ITEMS; ++i) {
        if (a.stock[i] < b.stock[i]) return false;
        for (int u = 0; u < kag::MAX_UNITS; ++u)
            if (trial.state.resources[seat].buffers[u][i] < original.state.resources[seat].buffers[u][i]) return false;
    }
    for (int i = 0; i < kag::N_CROPS; ++i) if (a.seeds[i] < b.seeds[i]) return false;
    return true;
}

int main(int argc, char** argv) {
    require(argc > 1, "usage: storage_headroom case.calendar [...]");
    for (int path = 1; path < argc; ++path) {
        auto c = read_case(argv[path]); certify_commitments(c);
        Outcome base{c.initial};
        std::vector<Outcome> prefixes;
        std::array<std::vector<Loss>, 2> losses;
        for (int t = 0; t < int(c.turns.size()); ++t) {
            prefixes.push_back(base);
            for (int p = 0; p < 2; ++p) {
                auto a = base.state.financial.accounts[p]; auto r = base.state.resources[p];
                const auto before = r.discarded;
                apply(a, r, c.turns[t].calendar[p].before_market, c.config.shed_capacity);
                Items delta{};
                for (int i = 0; i < kag::N_ITEMS; ++i) delta[i] = r.discarded[i] - before[i];
                if (sum(delta)) losses[p].push_back({t, 0, sum(delta), delta});
            }
            const auto before = base;
            step(c, t, base);
            for (int p = 0; p < 2; ++p) {
                Items delta{};
                auto a = before.state.financial.accounts[p]; auto r = before.state.resources[p];
                apply(a, r, c.turns[t].calendar[p].before_market, c.config.shed_capacity);
                for (int i = 0; i < kag::N_ITEMS; ++i) delta[i] = base.state.resources[p].discarded[i] - r.discarded[i];
                if (sum(delta)) losses[p].push_back({t, 1, sum(delta), delta});
            }
        }
        for (int seat = 0; seat < 2; ++seat) {
            const auto started = std::chrono::steady_clock::now();
            for (const auto loss : losses[seat]) {
                const int last = loss.turn - (loss.phase == 0);
                double best_gain = 0; int best_saved = 0, best_turn = -1, best_item = -1, best_quantity = 0;
                int trials = 0, feasible = 0, max_available = 0;
                for (int t = std::max(0, last - 3); t <= last; ++t) {
                    if (c.turns[t].original_orders[seat].count >= c.config.max_orders) continue;
                    auto own = prefixes[t].state.financial.accounts[seat];
                    auto resources = prefixes[t].state.resources[seat];
                    apply(own, resources, c.turns[t].calendar[seat].before_market, c.config.shed_capacity);
                    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                        max_available = std::max(max_available, own.stock[item]);
                        for (int q = 1; q <= std::min(loss.quantity, own.stock[item]); ++q) {
                            auto trial = prefixes[t]; ++trials;
                            const Edit edit{t, seat, item, q};
                            for (int future = t; future < int(c.turns.size()); ++future) step(c, future, trial, edit);
                            if (!preserves(trial, base, seat)) continue;
                            ++feasible;
                            const int saved = sum(base.state.resources[seat].discarded) - sum(trial.state.resources[seat].discarded);
                            const auto& a = trial.state.financial.accounts; const auto& b = base.state.financial.accounts;
                            const double gain = (a[seat].cash - a[seat ^ 1].cash) - (b[seat].cash - b[seat ^ 1].cash);
                            if (saved > 0 && gain > best_gain) {
                                best_gain = gain; best_saved = saved; best_turn = t; best_item = item; best_quantity = q;
                            }
                        }
                    }
                }
                std::printf("{\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"phase\":%d,\"lost\":%d,\"wheat\":%d,\"fertilizer\":%d,\"max_available\":%d,\"trials\":%d,\"feasible\":%d,\"best_gain\":%.0f,\"saved\":%d,\"edit_turn\":%d,\"item\":%d,\"quantity\":%d,\"cumulative_seconds\":%.6f}\n",
                            (unsigned long long)c.episode, seat, loss.turn, loss.phase, loss.quantity,
                            loss.discarded[kag::WHEAT], loss.discarded[kag::FERTILIZER], max_available, trials,
                            feasible, best_gain, best_saved, best_turn, best_item, best_quantity,
                            std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
            }
        }
    }
}
