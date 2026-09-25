// Raw Local-LB trace (tools/lb_trace_play.py) -> standard trace (load_replay format).
// Re-simulates the recorded actions in the C++ engine and checks the final money
// against the Python engine's. usage: finalize_trace in.raw out.txt
#include "source/world.hpp"
#include <cstdio>
#include <iostream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: finalize_trace in.raw out.txt\n";
        return 2;
    }
    std::ifstream in(argv[1]);
    Config c;
    int count = 0;
    std::string tag;
    in >> c.seed >> count >> tag;
    if (tag != "CONFIG") return std::cerr << "bad header\n", 1;
    in >> c.episode_steps >> c.board_size >> c.starting_money >> c.max_orders >> c.turns_per_day >> c.shed_capacity >>
        c.weed_chance >> c.shop_unlock_interval >> c.shop_sell_interval >> c.center_sell_interval >> c.hire_mult;
    std::vector<std::array<Action, 2>> turns(count);
    for (auto& turn : turns)
        for (Action& a : turn) {
            in >> a.n_units >> a.n_orders;
            for (int u = 0; u < a.n_units; ++u) {
                int op, arg;
                in >> op >> arg >> a.units[u].n;
                a.units[u].op = uint8_t(op), a.units[u].arg = uint8_t(arg);
            }
            for (int k = 0; k < a.n_orders; ++k) {
                int op, item;
                in >> op >> item >> a.orders[k].n;
                a.orders[k].op = uint8_t(op), a.orders[k].item = uint8_t(item);
            }
            a.finalize();
        }
    double money[2];
    in >> tag >> money[0] >> money[1];
    if (!in || tag != "MONEY") return std::cerr << "bad body " << argv[1] << '\n', 1;
    Sim sim(c);
    for (const auto& t : turns) sim.step(t[0], t[1]);
    for (int p = 0; p < 2; ++p)
        if (std::abs(sim.st.farms[p].money - money[p]) > 0.5) {
            std::fprintf(stderr, "%s: parity mismatch seat %d: C++ %.0f Python %.0f\n", argv[1], p, sim.st.farms[p].money, money[p]);
            return 1;
        }
    save_replay(argv[2], c, turns);
    return 0;
}
