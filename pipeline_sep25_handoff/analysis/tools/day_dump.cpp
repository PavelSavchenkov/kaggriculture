// One seat's day, hour by hour, for one product: successful harvests of it, deposits,
// units carried, shed stock, own and opponent sales (units @ average price), market
// price before the market, and the opponent's harvests of it.
// usage: day_dump trace seat day product
#include "source/world.hpp"
#include <cstdio>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 5) return std::fprintf(stderr, "usage: day_dump trace seat day product\n"), 2;
    const Replay replay = load_replay(argv[1]);
    const int seat = std::atoi(argv[2]), day = std::atoi(argv[3]), product = std::atoi(argv[4]);
    Sim sim(replay.config);
    std::printf("hour workers harvest_units(own) deposited carried_after shed_after | own sold | opp sold | opp harvested | price_before\n");
    for (size_t s = 0; s < replay.turns.size(); ++s) {
        if (sim.st.day > day) break;
        const Action& a0 = replay.turns[s][0];
        const Action& a1 = replay.turns[s][1];
        if (sim.st.day < day) {
            sim.step(a0, a1);
            continue;
        }
        const Sim before = sim;
        const auto san = sim.sanitize_joint_actions(a0, a1);
        const int price = sim.st.market.prices[product];
        double revenue[2] = {before.st.farms[0].sell_revenue, before.st.farms[1].sell_revenue};
        int sold0[2] = {before.st.farms[0].sold_units[product], before.st.farms[1].sold_units[product]};
        int produced0[2] = {before.st.farms[0].produced[product], before.st.farms[1].produced[product]};
        Sim units_only = step_without_night(sim, a0, a1);
        sim.step(a0, a1);
        int carried = 0, deposits = 0;
        const Farm& f = units_only.st.farms[seat];
        for (int u = 0; u < f.n_units; ++u) carried += f.inv[u][product];
        for (int u = 0; u < before.st.farms[seat].n_units; ++u)
            if (san[seat].units[u].op == OP_DROP || (san[seat].units[u].op == OP_PLACE && san[seat].units[u].arg == product))
                deposits += before.st.farms[seat].inv[u][product] - (san[seat].units[u].op == OP_PLACE ? before.st.farms[seat].inv[u][product] - san[seat].units[u].n : 0);
        const int sold[2] = {sim.st.farms[0].sold_units[product] - sold0[0], sim.st.farms[1].sold_units[product] - sold0[1]};
        std::printf("%2d %3d %4d %4d %4d %4d | %3d | %3d | %3d | %d\n", before.st.hour, before.st.farms[seat].n_units,
                    units_only.st.farms[seat].produced[product] - produced0[seat], deposits, carried,
                    int(units_only.st.farms[seat].shed[product]), sold[seat], sold[1 - seat], units_only.st.farms[1 - seat].produced[product] - produced0[1 - seat], price);
        (void)revenue;
    }
    return 0;
}
