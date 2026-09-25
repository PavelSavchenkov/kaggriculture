// Spatial dispersion of same-day cohorts: for each seat and day, the tiles planted that day
// (per crop type and all crops) and the animals placed that day; per cohort the tile count,
// quadrants spanned, MST length over the tiles, and mean shed distance. Also the tiles
// harvested (crops) on each day as a group. One row per cohort.
// usage: cohort_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
#include <map>
#include <sstream>

using namespace dc10;

namespace {
int mst(const std::vector<int>& cells) {
    const int n = int(cells.size());
    if (n < 2) return 0;
    std::vector<int> best(n, 1 << 20);
    std::vector<bool> in(n, false);
    best[0] = 0;
    int total = 0;
    for (int k = 0; k < n; ++k) {
        int u = -1;
        for (int i = 0; i < n; ++i)
            if (!in[i] && (u < 0 || best[i] < best[u])) u = i;
        in[u] = true;
        total += best[u];
        for (int i = 0; i < n; ++i)
            if (!in[i]) best[i] = std::min(best[i], dist(cells[u], cells[i]));
    }
    return total;
}
const char* KIND[] = {"wheat", "carrot", "tomato", "strawberry", "melon", "all_crops", "animals", "harvested"};
}

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: cohort_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,opp_label,group,day,kind,n,quadrants,mst,mean_shed_dist\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2], group;
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        fields >> group;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        std::vector<int> cohort[2][8];
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int day = sim.st.day;
            const auto san = sim.sanitize_joint_actions(replay.turns[s][0], replay.turns[s][1]);
            for (int p = 0; p < 2; ++p) {
                const Farm& f = sim.st.farms[p];
                for (int u = 0; u < f.n_units; ++u) {
                    const auto& a = san[p].units[u];
                    const int cell = cell_of(f.pos_x[u], f.pos_y[u]);
                    const Tile& t = f.tiles[f.pos_y[u]][f.pos_x[u]];
                    if (a.op == OP_PLANT) cohort[p][a.arg].push_back(cell), cohort[p][5].push_back(cell);
                    if (a.op == OP_PLACE && is_animal(a.arg) && (t.kind == T_COOP || t.kind == T_PASTURE)) cohort[p][6].push_back(cell);
                    if (a.op == OP_HARVEST && t.kind == T_PLANT) cohort[p][7].push_back(cell);
                }
            }
            if (sim.st.hour == HOURS - 1 || s + 1 == replay.turns.size())
                for (int p = 0; p < 2; ++p)
                    for (int k = 0; k < 8; ++k) {
                        auto& c = cohort[p][k];
                        if (c.size() >= 2) {
                            bool quad[4]{};
                            int dsum = 0;
                            for (int x : c) quad[quadrant_of(cell_x(x), cell_y(x), BOARD)] = true, dsum += shed_dist(x);
                            out << trace << ',' << p << ',' << labels[p] << ',' << labels[1 - p] << ',' << group << ',' << day << ','
                                << KIND[k] << ',' << c.size() << ',' << quad[0] + quad[1] + quad[2] + quad[3] << ',' << mst(c) << ','
                                << double(dsum) / c.size() << '\n';
                        }
                        c.clear();
                    }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
        }
    }
    return 0;
}
