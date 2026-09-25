// Shops unlocked in each trace: one row per trace and unlock day (3, 6, ..., 24) with the count of
// each shop type unlocked so far (replaying the recorded actions: the draw depends on empty tiles).
// usage: shops_audit list.txt out.csv   (list line: trace ...; duplicates skipped)
#include "source/world.hpp"
#include <iostream>
#include <set>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) return std::cerr << "usage: shops_audit list.txt out.csv\n", 2;
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    const char* names[N_SHOPS] = {"bakery", "brunch", "farmers", "icecream", "petcafe", "pizza", "smoothie", "yarn"};
    out << "trace,day";
    for (const char* n : names) out << ',' << n;
    out << '\n';
    std::set<std::string> seen;
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace;
        if (!(fields >> trace) || !seen.insert(trace).second) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        // Shop draws share the night RNG stream with weed spawns (one draw per empty tile on both
        // farms), so the recorded actions must be replayed.
        size_t step = 0;
        for (int day = 0; day <= LAST_DAY; ++day) {
            for (int h = 0; h < HOURS && !sim.st.done && step < replay.turns.size(); ++h, ++step)
                sim.step(replay.turns[step][0], replay.turns[step][1]);
            if ((day + 1) % 3 == 0 && day + 1 <= 24) {
                int count[N_SHOPS]{};
                for (int s = 0; s < sim.st.n_shops; ++s) ++count[sim.st.shops[s]];
                out << trace << ',' << day + 1;
                for (int c : count) out << ',' << c;
                out << '\n';
            }
        }
    }
    return 0;
}
