// Worker trips: each worker's day split at shed interactions (successful PICKUP, DROP or
// PLACE of items). Per trip: moves, farm actions, distinct tiles worked, animal/crop mix,
// the Manhattan MST over the shed start tile and the tiles worked (route lower-bound proxy),
// start hour and cargo carried at the end of the trip. Also per worker-day: first and last
// working hour. Output: one row per trip.
// usage: trip_audit list.txt out.csv   (list line: trace label0 label1 [group])
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

struct Trip {
    int start_hour = -1, start_cell = -1, moves = 0, actions = 0, animal_actions = 0, crop_actions = 0, idle = 0;
    std::vector<int> tiles;
    bool active() const { return moves || actions; }
};
}

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: trip_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,opp_label,group,day,worker,start_hour,end_hour,moves,actions,animal_actions,crop_actions,tiles,mst,cargo_end,ends_at_shed\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, labels[2], group;
        if (!(fields >> trace >> labels[0] >> labels[1])) continue;
        fields >> group;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        Trip trips[2][MAX_UNITS];
        auto flush = [&](int p, int u, int day, int end_hour, int cargo, bool at_shed) {
            Trip& t = trips[p][u];
            if (t.active()) {
                std::vector<int> cells = t.tiles;
                cells.insert(cells.begin(), t.start_cell);
                std::sort(cells.begin() + 1, cells.end());
                cells.erase(std::unique(cells.begin() + 1, cells.end()), cells.end());
                const int distinct = int(cells.size()) - 1;
                out << trace << ',' << p << ',' << labels[p] << ',' << labels[1 - p] << ',' << group << ',' << day << ',' << u << ','
                    << t.start_hour << ',' << end_hour << ',' << t.moves << ',' << t.actions << ',' << t.animal_actions << ','
                    << t.crop_actions << ',' << distinct << ',' << mst(cells) << ',' << cargo << ',' << at_shed << '\n';
            }
            t = Trip{};
        };
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int day = sim.st.day, hour = sim.st.hour;
            const auto san = sim.sanitize_joint_actions(replay.turns[s][0], replay.turns[s][1]);
            for (int p = 0; p < 2; ++p) {
                const Farm& f = sim.st.farms[p];
                for (int u = 0; u < f.n_units; ++u) {
                    Trip& t = trips[p][u];
                    const int cell = cell_of(f.pos_x[u], f.pos_y[u]);
                    if (t.start_cell < 0) t.start_cell = cell, t.start_hour = hour;
                    const auto& a = san[p].units[u];
                    const Tile& tile = f.tiles[f.pos_y[u]][f.pos_x[u]];
                    const bool shed_op = shed_access(cell) && (a.op == OP_PICKUP || a.op == OP_DROP ||
                                                               (a.op == OP_PLACE && !(is_animal(a.arg) && (tile.kind == T_COOP || tile.kind == T_PASTURE))));
                    if (shed_op) {
                        int cargo = 0;
                        for (int i = 0; i < N_ITEMS; ++i) cargo += f.inv[u][i];
                        flush(p, u, day, hour, cargo, true);
                        t.start_cell = cell, t.start_hour = hour;
                        continue;
                    }
                    if (a.op >= OP_NORTH && a.op <= OP_WEST) ++t.moves;
                    else if (a.op != OP_PASS) {
                        ++t.actions;
                        (tile.kind == T_COOP || tile.kind == T_PASTURE ? t.animal_actions : t.crop_actions) += 1;
                        t.tiles.push_back(cell);
                    }
                }
            }
            const bool end = hour == HOURS - 1 || s + 1 == replay.turns.size();
            if (end)
                for (int p = 0; p < 2; ++p) {
                    const Farm& f = sim.st.farms[p];
                    for (int u = 0; u < f.n_units; ++u) {
                        int cargo = 0;
                        for (int i = 0; i < N_ITEMS; ++i) cargo += f.inv[u][i];
                        flush(p, u, day, hour, cargo, false);
                    }
                    for (int u = 0; u < MAX_UNITS; ++u) trips[p][u] = Trip{};
                }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
        }
    }
    return 0;
}
