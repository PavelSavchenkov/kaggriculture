// One seat's day from a replay: dawn crops (tile crop age yield) and each HARVEST / FEED / PICKUP / SELL of wheat that day (hour,
// unit, tile, crop there, units gained). usage: day_harvests trace seat day
#include "source/world.hpp"
#include <iostream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 4) return std::cerr << "usage: day_harvests trace seat day\n", 2;
    const Replay replay = load_replay(argv[1]);
    const int seat = std::atoi(argv[2]), day = std::atoi(argv[3]);
    Sim sim(replay.config);
    for (size_t s = 0; s < replay.turns.size() && !sim.st.done; ++s) {
        const int d = sim.st.step / HOURS, h = sim.st.step % HOURS;
        const Farm& f = sim.st.farms[seat];
        if (d == day && h == 0) {
            std::cout << "dawn d" << day << " money " << f.money << " shed wheat " << int(f.shed[WHEAT]) << " fert " << int(f.shed[FERTILIZER]) << "\ncrops:";
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    const Tile& t = f.tiles[y][x];
                    if (t.kind == T_PLANT) std::cout << ' ' << y * BOARD + x << ":c" << int(t.what) << ":a" << (day - t.planted_day) << ":y" << int(t.yield_units);
                }
            std::cout << '\n';
        }
        if (d == day) {
            const Action& a = replay.turns[s][seat];
            for (int u = 0; u < a.n_units && u < f.n_units; ++u) {
                const UnitAction& x = a.units[u];
                if (x.op != OP_HARVEST && x.op != OP_FEED && !(x.op == OP_PICKUP && x.arg == WHEAT)) continue;
                const int tile = f.pos_y[u] * BOARD + f.pos_x[u];
                const Tile& t = f.tiles[f.pos_y[u]][f.pos_x[u]];
                std::cout << "h" << h << " u" << u << " op" << int(x.op) << " tile " << tile << " kind " << int(t.kind) << " what " << int(t.what)
                          << " yield " << int(t.yield_units) << " wheat_in_pocket " << int(f.inv[u][WHEAT]) << '\n';
            }
        }
        sim.step(replay.turns[s][0], replay.turns[s][1]);
        if (d > day) break;
    }
    return 0;
}
