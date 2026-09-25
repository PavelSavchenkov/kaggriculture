// Melons per seat: plants by planting day with their shed distance, and on days 10-13 the hour of
// each melon harvest (yield taken) and the seat's melon sales by hour.
// usage: melon_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: melon_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,event,day,hour,units,dist\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2];
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int day = int(s / HOURS), hour = int(s % HOURS);
            if (hour == 0 && day == 10)  // melon plants at dawn of day 10: planting day and shed distance
                for (int p = 0; p < 2; ++p)
                    for (int y = 0; y < BOARD; ++y)
                        for (int x = 0; x < BOARD; ++x) {
                            const Tile& t = sim.st.farms[p].tiles[y][x];
                            if (t.kind == T_PLANT && t.what == MELON)
                                out << trace << ',' << p << ',' << labels[p] << ",plant," << t.planted_day << ",0,1," << shed_dist(cell_of(x, y)) << '\n';
                        }
            int before[2][BOARD][BOARD];
            int sold0[2] = {sim.st.farms[0].sold_units[MELON], sim.st.farms[1].sold_units[MELON]};
            for (int p = 0; p < 2; ++p)
                for (int y = 0; y < BOARD; ++y)
                    for (int x = 0; x < BOARD; ++x) {
                        const Tile& t = sim.st.farms[p].tiles[y][x];
                        before[p][y][x] = t.kind == T_PLANT && t.what == MELON ? t.yield_units : -1;
                    }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            if (day < 10 || day > 13) continue;
            for (int p = 0; p < 2; ++p) {
                for (int y = 0; y < BOARD; ++y)
                    for (int x = 0; x < BOARD; ++x) {
                        const Tile& t = sim.st.farms[p].tiles[y][x];
                        const bool still = t.kind == T_PLANT && t.what == MELON;
                        if (before[p][y][x] > 0 && !still)
                            out << trace << ',' << p << ',' << labels[p] << ",harvest," << day << ',' << hour << ',' << before[p][y][x] << ','
                                << shed_dist(cell_of(x, y)) << '\n';
                    }
                const int sold = sim.st.farms[p].sold_units[MELON] - sold0[p];
                if (sold > 0) out << trace << ',' << p << ',' << labels[p] << ",sell," << day << ',' << hour << ',' << sold << ",0\n";
            }
        }
    }
    return 0;
}
