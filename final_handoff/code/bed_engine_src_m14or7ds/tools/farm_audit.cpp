// Farm growth per dawn for one seat of each trace: land, plants by crop, animals by
// species, money, hires in the previous day, cumulative wheat produced and sold, total spend, units discarded (total and by item), units sold, sale revenue, shop types so far. Long format for day-by-day comparisons.
// usage: farm_audit list.txt out.csv   (list line: trace seat label)
#include "source/world.hpp"
#include <iostream>
#include <sstream>
#include "tools/shop_crn.hpp"

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: farm_audit list.txt out.csv\n";
        return 2;
    }
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,day,money,land,wheat,carrot,tomato,strawberry,melon,goose,cow,sheep,workers,shed_wheat,shed_total,produced_wheat,sold_wheat,spend,discarded,sold_units,revenue,discarded_by_item,discarded_day,discarded_night,shops\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, label;
        int seat = 0;
        if (!(fields >> trace >> seat >> label)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int crn_shops = 0;  // SHOP_CRN: shops fixed so far
        int workers = 0, discarded_day = 0, discarded_night = 0;
        for (size_t s = 0; s <= replay.turns.size(); ++s) {
            const Farm& f = sim.st.farms[seat];
            if (s % HOURS == HOURS - 1) workers = f.n_units;  // before hired hands leave at day end
            if (s % HOURS == 0) {
                int crops[N_CROPS]{}, animals[N_ANIMALS]{};
                for (int y = 0; y < BOARD; ++y)
                    for (int x = 0; x < BOARD; ++x) {
                        const Tile& t = f.tiles[y][x];
                        if (t.kind == T_PLANT) ++crops[t.what];
                        if (t.has_animal) ++animals[t.what - GOOSE];
                    }
                out << trace << ',' << seat << ',' << label << ',' << s / HOURS << ',' << f.money << ',' << f.n_quadrants;
                for (int c : crops) out << ',' << c;
                for (int a : animals) out << ',' << a;
                out << ',' << workers << ',' << int(f.shed[WHEAT]) << ',' << f.shed_total << ',' << f.produced[WHEAT] << ','
                    << f.sold_units[WHEAT] << ',' << f.total_spend;
                int discarded = 0, sold = 0;
                for (int i = 0; i < N_ITEMS; ++i) discarded += f.discarded[i], sold += f.sold_units[i];
                out << ',' << discarded << ',' << sold << ',' << f.sell_revenue << ',';
                for (int i = 0; i < N_ITEMS; ++i) out << (i ? ";" : "") << f.discarded[i];
                out << ',' << discarded_day << ',' << discarded_night << ',';
                for (int k = 0; k < sim.st.n_shops; ++k) out << (k ? ";" : "") << int(sim.st.shops[k]);
                out << '\n';
            }
            if (s < replay.turns.size()) {
                int before = 0, after = 0;
                for (int i = 0; i < N_ITEMS; ++i) before += sim.st.farms[seat].discarded[i];
                sim.step(replay.turns[s][0], replay.turns[s][1]);
                shop_crn(sim, crn_shops);
                for (int i = 0; i < N_ITEMS; ++i) after += sim.st.farms[seat].discarded[i];
                (s % HOURS == HOURS - 1 ? discarded_night : discarded_day) += after - before;  // night: the last step's deposit
            }
        }
    }
    return 0;
}
