// Market of one product through a trace: per day, the dawn inventory and price, units sold by each
// seat, and consumption (shops and town) = dawn inventory + sales - next dawn inventory.
// usage: market_audit trace.txt product_index
#include "source/world.hpp"
#include <iostream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: market_audit trace product\n", 2;
    const Replay replay = load_replay(argv[1]);
    const int q = std::atoi(argv[2]);
    Sim sim(replay.config);
    std::printf("day dawn_inv dawn_price sold0 sold1 consumed shops\n");
    int inv0 = sim.st.market.inventory[q], price0 = sim.st.market.prices[q], s0[2] = {0, 0};
    for (size_t s = 0; s < replay.turns.size(); ++s) {
        sim.step(replay.turns[s][0], replay.turns[s][1]);
        if ((s + 1) % HOURS == 0 || s + 1 == replay.turns.size()) {
            const int day = int(s / HOURS);
            const int sold[2] = {sim.st.farms[0].sold_units[q] - s0[0], sim.st.farms[1].sold_units[q] - s0[1]};
            const int inv = sim.st.market.inventory[q];
            std::printf("%3d %8d %10d %5d %5d %8d %5d\n", day, inv0, price0, sold[0], sold[1], inv0 + sold[0] + sold[1] - inv, sim.st.n_shops);
            inv0 = inv, price0 = sim.st.market.prices[q];
            s0[0] = sim.st.farms[0].sold_units[q], s0[1] = sim.st.farms[1].sold_units[q];
        }
    }
    return 0;
}
