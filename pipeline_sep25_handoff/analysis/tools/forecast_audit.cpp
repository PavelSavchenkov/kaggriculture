// Opponent-forecast audit: for every seat and dawn, the seat's publicly visible supply per product
// (held product on animals, yield on harvestable crops: what the other player sees), its actual
// units sold that day, and units sold in hours 0-11 of that day.
// usage: forecast_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: forecast_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,group,day,product,visible,sold,sold_morning\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2], group;
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        fields >> group;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int visible[2][N_PRODUCTS]{}, sold0[2][N_PRODUCTS]{}, morning[2][N_PRODUCTS]{};
        for (size_t s = 0; s <= replay.turns.size(); ++s) {
            const int hour = int(s % HOURS), day = int(s / HOURS);
            const bool dawn = hour == 0 || s == replay.turns.size();
            if (dawn && s > 0) {
                const int prev = day - (hour == 0 ? 1 : 0);
                for (int p = 0; p < 2; ++p)
                    for (int q = 0; q < N_PRODUCTS; ++q)
                        out << trace << ',' << p << ',' << labels[p] << ',' << group << ',' << prev << ',' << q << ',' << visible[p][q]
                            << ',' << sim.st.farms[p].sold_units[q] - sold0[p][q] << ',' << morning[p][q] << '\n';
            }
            if (s == replay.turns.size()) break;
            if (hour == 0)
                for (int p = 0; p < 2; ++p) {
                    const Farm& f = sim.st.farms[p];
                    std::fill_n(visible[p], N_PRODUCTS, 0);
                    for (int y = 0; y < BOARD; ++y)
                        for (int x = 0; x < BOARD; ++x) {
                            const Tile& t = f.tiles[y][x];
                            if (t.has_animal) visible[p][ANIMALS[t.what - GOOSE].product] += t.yield_units;
                            else if (t.kind == T_PLANT && day - t.planted_day >= CROPS[t.what].first_yield_day) visible[p][t.what] += t.yield_units;
                        }
                    for (int q = 0; q < N_PRODUCTS; ++q) sold0[p][q] = f.sold_units[q], morning[p][q] = 0;
                }
            int before[2][N_PRODUCTS];
            for (int p = 0; p < 2; ++p) std::copy_n(sim.st.farms[p].sold_units, N_PRODUCTS, before[p]);
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            if (hour < 12)
                for (int p = 0; p < 2; ++p)
                    for (int q = 0; q < N_PRODUCTS; ++q) morning[p][q] += sim.st.farms[p].sold_units[q] - before[p][q];
        }
    }
    return 0;
}
