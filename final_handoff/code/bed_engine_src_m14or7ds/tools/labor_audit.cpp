// Worker labor per day for one seat of each trace: unit actions by type (moves, pickup, drop,
// place, plant, water, harvest, fertilize, dig, build, feed, collect fertilizer, care, pass) and
// workers on the farm, days 10-27. Where the day's labor (and so the Fibonacci wages) goes.
// usage: labor_audit list.txt out.csv   (list line: trace seat label)
#include "source/world.hpp"
#include <iostream>
#include <sstream>
#include "tools/shop_crn.hpp"

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: labor_audit list.txt out.csv\n";
        return 2;
    }
    const char* names[] = {"pass", "move", "pickup", "drop", "place", "plant", "water", "harvest", "fertilize",
                           "dig", "build", "feed", "collect", "care"};
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,label,worker_hours";
    for (auto name : names) out << ',' << name;
    out << ",pass_h0_5,pass_h6_11,pass_h12_17,pass_h18_23,pass_farmer,idle_workers_end,pass_h20,pass_h21,pass_h22,pass_h23\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, label;
        int seat = 0;
        if (!(fields >> trace >> seat >> label)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int crn_shops = 0;  // SHOP_CRN: shops fixed so far
        double count[14]{}, worker_hours = 0, pass_by[4]{}, pass_farmer = 0, idle_end = 0, pass_late[4]{};
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int day = int(s / HOURS);
            if (day >= 10 && day <= 27) {
                const Action& a = replay.turns[s][seat];
                const int units = sim.st.farms[seat].n_units;
                worker_hours += units;
                for (int u = 0; u < units; ++u) {
                    const int op = u < a.n_units ? a.units[u].op : OP_PASS;
                    const int k = op == OP_PASS || op >= OP_INVALID ? 0
                                  : op <= OP_WEST              ? 1
                                  : op == OP_BUILD_COOP || op == OP_BUILD_PASTURE ? 10
                                  : op == OP_FEED              ? 11
                                  : op == OP_COLLECT_FERTILIZER ? 12
                                  : op == OP_CARE              ? 13
                                                               : op - OP_PICKUP + 2;
                    ++count[k];
                    if (!k) {
                        pass_by[(s % HOURS) / 6] += 1;
                        if (s % HOURS >= 20) pass_late[s % HOURS - 20] += 1;
                        pass_farmer += u == 0;
                    }
                }
                if (s % HOURS == HOURS - 1)  // workers passing through the whole last 6 hours
                    for (int u = 0; u < units; ++u) {
                        bool idle = true;
                        for (size_t t = s - 5; t <= s && idle; ++t)
                            idle = u >= replay.turns[t][seat].n_units || replay.turns[t][seat].units[u].op == OP_PASS;
                        idle_end += idle;
                    }
            }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            shop_crn(sim, crn_shops);
        }
        out << trace << ',' << seat << ',' << label << ',' << worker_hours / 18;
        for (double c : count) out << ',' << c / 18;  // per day
        for (double c : pass_by) out << ',' << c / 18;
        out << ',' << pass_farmer / 18 << ',' << idle_end / 18;
        for (double c : pass_late) out << ',' << c / 18;
        out << '\n';
    }
    return 0;
}
