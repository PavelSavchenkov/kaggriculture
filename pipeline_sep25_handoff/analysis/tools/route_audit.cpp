// Worker routes per seat and day (days 10-28): for each worker, moves made, work actions, and the
// Manhattan minimum spanning tree over its start tile and the tiles where it worked (a lower
// bound on the walking any order of those jobs needs). Summed per seat-day.
// usage: route_audit list.txt out.csv   (list line: trace label0 label1 [group])
#include "source/world.hpp"
#include <iostream>
#include <set>
#include <sstream>

using namespace dc10;

namespace {
int mst(const std::vector<int>& cells) {
    const int n = int(cells.size());
    if (n < 2) return 0;
    std::vector<int> best(n, 1 << 30);
    std::vector<bool> in(n, false);
    best[0] = 0;
    int total = 0;
    for (int k = 0; k < n; ++k) {
        int u = -1;
        for (int i = 0; i < n; ++i)
            if (!in[i] && (u < 0 || best[i] < best[u])) u = i;
        in[u] = true, total += best[u];
        for (int i = 0; i < n; ++i)
            if (!in[i]) best[i] = std::min(best[i], std::abs(cell_x(cells[u]) - cell_x(cells[i])) + std::abs(cell_y(cells[u]) - cell_y(cells[i])));
    }
    return total;
}
}

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: route_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,day,workers,moves,work,mst,tiles\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2];
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int moves[2][MAX_UNITS]{}, work[2][MAX_UNITS]{}, start[2][MAX_UNITS];
        std::vector<int> tiles[2][MAX_UNITS];
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int hour = int(s % HOURS), day = int(s / HOURS);
            if (hour == 0)
                for (int p = 0; p < 2; ++p)
                    for (int u = 0; u < MAX_UNITS; ++u) moves[p][u] = work[p][u] = 0, start[p][u] = -1, tiles[p][u].clear();
            for (int p = 0; p < 2; ++p) {
                const Farm& f = sim.st.farms[p];
                const Action& a = replay.turns[s][p];
                for (int u = 0; u < std::min<int>(f.n_units, a.n_units); ++u) {
                    const int cell = cell_of(f.pos_x[u], f.pos_y[u]);
                    if (start[p][u] < 0) start[p][u] = cell;
                    const int op = a.units[u].op;
                    if (op >= OP_NORTH && op <= OP_WEST) ++moves[p][u];
                    else if (op != OP_PASS) {
                        ++work[p][u];
                        if (op != OP_PICKUP && op != OP_DROP) tiles[p][u].push_back(cell);
                    }
                }
            }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            if (hour == HOURS - 1 && day >= 10 && day <= 28)
                for (int p = 0; p < 2; ++p) {
                    int W = 0, M = 0, K = 0, T = 0, S = 0;
                    for (int u = 0; u < MAX_UNITS; ++u) {
                        if (start[p][u] < 0) continue;
                        ++W, M += moves[p][u], K += work[p][u];
                        std::vector<int> cells = {start[p][u]};
                        std::set<int> seen(tiles[p][u].begin(), tiles[p][u].end());
                        cells.insert(cells.end(), seen.begin(), seen.end());
                        T += int(seen.size()), S += mst(cells);
                    }
                    out << trace << ',' << p << ',' << labels[p] << ',' << day << ',' << W << ',' << M << ',' << K << ',' << S << ',' << T << '\n';
                }
        }
    }
    return 0;
}
