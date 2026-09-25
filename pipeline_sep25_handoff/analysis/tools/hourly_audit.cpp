// Hour-by-hour opponent sales for forecasting: per seat, day and product, the dawn visible supply
// (held product on animals, yield on harvestable crops), the shed stock at dawn (private), and
// units sold at each hour 0-23.
// usage: hourly_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: hourly_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,day,product,visible,shed";
    for (int h = 0; h < HOURS; ++h) out << ",h" << h;
    out << '\n';
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2];
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int visible[2][N_PRODUCTS]{}, shed[2][N_PRODUCTS]{}, sold[2][N_PRODUCTS][HOURS]{};
        for (size_t s = 0; s <= replay.turns.size(); ++s) {
            const int hour = int(s % HOURS), day = int(s / HOURS);
            if ((hour == 0 && s > 0) || s == replay.turns.size()) {
                const int prev = s == replay.turns.size() && hour != 0 ? day : day - 1;
                for (int p = 0; p < 2; ++p)
                    for (int q = 0; q < N_PRODUCTS; ++q) {
                        out << trace << ',' << p << ',' << labels[p] << ',' << prev << ',' << q << ',' << visible[p][q] << ',' << shed[p][q];
                        for (int h = 0; h < HOURS; ++h) out << ',' << sold[p][q][h];
                        out << '\n';
                    }
            }
            if (s == replay.turns.size()) break;
            if (hour == 0) {
                std::fill_n(&sold[0][0][0], 2 * N_PRODUCTS * HOURS, 0);
                for (int p = 0; p < 2; ++p) {
                    const Farm& f = sim.st.farms[p];
                    std::fill_n(visible[p], N_PRODUCTS, 0);
                    for (int y = 0; y < BOARD; ++y)
                        for (int x = 0; x < BOARD; ++x) {
                            const Tile& t = f.tiles[y][x];
                            if (t.has_animal) visible[p][ANIMALS[t.what - GOOSE].product] += t.yield_units;
                            else if (t.kind == T_PLANT && day - t.planted_day >= CROPS[t.what].first_yield_day) visible[p][t.what] += t.yield_units;
                        }
                    for (int q = 0; q < N_PRODUCTS; ++q) shed[p][q] = f.shed[q];
                }
            }
            int before[2][N_PRODUCTS];
            for (int p = 0; p < 2; ++p) std::copy_n(sim.st.farms[p].sold_units, N_PRODUCTS, before[p]);
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            for (int p = 0; p < 2; ++p)
                for (int q = 0; q < N_PRODUCTS; ++q) sold[p][q][hour] += sim.st.farms[p].sold_units[q] - before[p][q];
        }
    }
    return 0;
}
