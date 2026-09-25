// Layout compactness per seat and day: tiles with successful farm work, their mean shed
// distance, and the Manhattan minimum spanning tree over them plus the shed access tiles
// (a lower bound proxy for the walking a day's work needs), next to the actual moves.
// usage: layout_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
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
}

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: layout_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,opp_label,group,day,worked_tiles,actions,moves,mst,mean_shed_dist,animal_tiles,crop_tiles\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2], group;
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        fields >> group;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        bool worked[2][BOARD * BOARD]{};
        int actions[2]{}, moves[2]{};
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int day = sim.st.day;
            const auto san = sim.sanitize_joint_actions(replay.turns[s][0], replay.turns[s][1]);
            for (int p = 0; p < 2; ++p) {
                const Farm& f = sim.st.farms[p];
                for (int u = 0; u < f.n_units; ++u) {
                    const int op = san[p].units[u].op;
                    if (op >= OP_NORTH && op <= OP_WEST) ++moves[p];
                    const bool farm = op == OP_PLANT || op == OP_WATER || op == OP_HARVEST || op == OP_FERTILIZE || op == OP_DIG ||
                                      op == OP_BUILD_COOP || op == OP_BUILD_PASTURE || op == OP_FEED || op == OP_CARE ||
                                      op == OP_COLLECT_FERTILIZER || (op == OP_PLACE && is_animal(san[p].units[u].arg) &&
                                                                     (f.tiles[f.pos_y[u]][f.pos_x[u]].kind == T_COOP || f.tiles[f.pos_y[u]][f.pos_x[u]].kind == T_PASTURE));
                    if (farm) ++actions[p], worked[p][cell_of(f.pos_x[u], f.pos_y[u])] = true;
                }
            }
            const bool end = sim.st.hour == HOURS - 1 || s + 1 == replay.turns.size();
            if (end)
                for (int p = 0; p < 2; ++p) {
                    const Farm& f = sim.st.farms[p];
                    std::vector<int> cells;
                    int dsum = 0, n = 0, animals = 0, crops = 0;
                    for (int c = 0; c < BOARD * BOARD; ++c)
                        if (worked[p][c]) {
                            cells.push_back(c), dsum += shed_dist(c), ++n;
                            const Tile& t = f.tiles[cell_y(c)][cell_x(c)];
                            animals += t.kind == T_COOP || t.kind == T_PASTURE, crops += t.kind == T_PLANT || t.kind == T_EMPTY || t.kind == T_WEED;
                        }
                    for (int c : {44, 45, 54, 55})
                        if (f.tiles[cell_y(c)][cell_x(c)].kind != T_LOCKED || c == 44) cells.push_back(c);
                    out << trace << ',' << p << ',' << labels[p] << ',' << labels[1 - p] << ',' << group << ',' << day << ',' << n << ','
                        << actions[p] << ',' << moves[p] << ',' << mst(cells) << ',' << (n ? double(dsum) / n : 0) << ',' << animals << ','
                        << crops << '\n';
                    std::fill_n(worked[p], BOARD * BOARD, false);
                    actions[p] = moves[p] = 0;
                }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
        }
    }
    return 0;
}
