// Land use per quadrant at given dawns for one seat of each trace: quadrant rank in purchase
// order (0 = home, 3 = 4th bought), whether owned, and tiles by crop, animal species, empty
// structures, weeds and empty land. Long format for comparing what top teams put on new land.
// usage: quadrant_audit list.txt out.csv [days, default 11,13,15,18,21,24]   (list line: trace seat label)
#include "source/world.hpp"
#include <iostream>
#include <sstream>
#include "tools/shop_crn.hpp"

using namespace dc10;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: quadrant_audit list.txt out.csv [days]\n";
        return 2;
    }
    std::vector<int> days = {11, 13, 15, 18, 21, 24};
    if (argc > 3) {
        days.clear();
        std::stringstream in(argv[3]);
        for (std::string d; std::getline(in, d, ',');) days.push_back(std::stoi(d));
    }
    int rank[4] = {0, 0, 0, 0};  // quadrant -> purchase rank
    for (int k = 0; k < 3; ++k) rank[LAND_ORDER[k]] = k + 1;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,day,rank,owned,wheat,carrot,tomato,strawberry,melon,goose,cow,sheep,coop,pasture,weed,empty\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, label;
        int seat = 0;
        if (!(fields >> trace >> seat >> label)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int crn_shops = 0;  // SHOP_CRN: shops fixed so far
        for (size_t s = 0; s <= replay.turns.size(); ++s) {
            const int day = int(s / HOURS);
            if (s % HOURS == 0 && std::find(days.begin(), days.end(), day) != days.end()) {
                const Farm& f = sim.st.farms[seat];
                int count[4][12]{};
                bool owned[4]{};
                for (int y = 0; y < BOARD; ++y)
                    for (int x = 0; x < BOARD; ++x) {
                        const Tile& t = f.tiles[y][x];
                        const int q = quadrant_of(x, y, BOARD);
                        if (t.kind != T_LOCKED) owned[q] = true;
                        if (t.kind == T_PLANT) ++count[q][t.what];
                        else if (t.has_animal) ++count[q][N_CROPS + t.what - GOOSE];
                        else if (t.kind == T_COOP) ++count[q][8];
                        else if (t.kind == T_PASTURE) ++count[q][9];
                        else if (t.kind == T_WEED) ++count[q][10];
                        else if (t.kind == T_EMPTY) ++count[q][11];
                    }
                for (int q = 0; q < 4; ++q) {
                    out << trace << ',' << seat << ',' << label << ',' << day << ',' << rank[q] << ',' << owned[q];
                    for (int c : count[q]) out << ',' << c;
                    out << '\n';
                }
            }
            if (s < replay.turns.size()) sim.step(replay.turns[s][0], replay.turns[s][1]);
            shop_crn(sim, crn_shops);
        }
    }
    return 0;
}
