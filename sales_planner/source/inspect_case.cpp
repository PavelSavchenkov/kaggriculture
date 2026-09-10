#include "baseline.hpp"
#include "case.hpp"
using namespace sales_planner;
int main(int argc, char** argv) {
    require(argc == 5, "usage: inspect_case calendar seat from to");
    auto c = read_case(argv[1]); certify_commitments(c);
    const int seat = std::atoi(argv[2]), from = std::atoi(argv[3]), to = std::atoi(argv[4]);
    auto state = c.initial.financial; auto resources = c.initial.resources;
    const auto rules = rules_for(c.config);
    std::vector<CalendarTurn> own;
    for (const auto& turn : c.turns) own.push_back(turn.calendar[seat]);
    for (int t = 0; t <= to; ++t) {
        const auto& turn = c.turns[t]; state.n_shops = turn.n_shops; state.shops = turn.shops;
        if (t >= from) {
            std::printf("turn%d cash%.0f stock", t, state.accounts[seat].cash);
            for (auto n : state.accounts[seat].stock) std::printf(" %d", n);
            std::printf(" seeds"); for (auto n : state.accounts[seat].seeds) std::printf(" %d", n);
            std::printf("\n");
            for (const auto e : own[t].before_market)
                std::printf(" event flow%d item%d buffer%d n%d\n", int(e.flow), e.item, e.buffer, e.quantity);
        }
        for (int p = 0; p < 2; ++p) apply(state.accounts[p], resources[p], turn.calendar[p].before_market, rules.capacity);
        PlannerObservation obs{t, state.accounts[seat], resources[seat], state.inventory, state.shops, state.n_shops, state.accounts[seat ^ 1].cash};
        auto orders = turn.original_orders;
        orders[seat] = baseline_orders(obs, own, rules, {4,4,4,false});
        if (t >= from) {
            auto demand = needs(obs, own, 4);
            std::printf(" target seeds"); for (auto n : demand.seeds) std::printf(" %d", n);
            std::printf("\n");
            for (int slot = 0; slot < orders[seat].count; ++slot) {
                const auto o = orders[seat].values[slot]; std::printf(" order %d op%d item%d n%d\n", slot, o.op, o.item, o.n);
            }
        }
        trade(state, orders, rules); consume(state, rules);
        for (int p = 0; p < 2; ++p) apply(state.accounts[p], resources[p], turn.calendar[p].after_market, rules.capacity);
        if (t >= from) {
            std::printf(" missing"); for (auto n : resources[seat].missing) std::printf(" %d", n);
            std::printf("\n");
        }
        advance(state, rules);
    }
}
