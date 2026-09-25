// Farm growth per dawn for one seat of each trace: land, plants by crop, animals by
// species, money, hires in the previous day, cumulative wheat produced and sold, total spend. Long format for day-by-day comparisons.
// usage: farm_audit list.txt out.csv   (list line: trace seat label)
#include "source/world.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: farm_audit list.txt out.csv\n";
        return 2;
    }
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,day,money,land,wheat,carrot,tomato,strawberry,melon,goose,cow,sheep,workers,shed_wheat,shed_total,produced_wheat,sold_wheat,spend\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, label;
        int seat = 0;
        if (!(fields >> trace >> seat >> label)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int workers = 0;
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
                    << f.sold_units[WHEAT] << ',' << f.total_spend << '\n';
            }
            if (s < replay.turns.size()) sim.step(replay.turns[s][0], replay.turns[s][1]);
        }
    }
    return 0;
}
