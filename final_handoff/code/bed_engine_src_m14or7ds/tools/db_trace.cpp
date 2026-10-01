// Builds engine traces from action-only episodes (scripts/import_replay_db.py): re-simulates
// each episode from its seed and both seats' actions, checks money and market inventory at
// every recorded daily snapshot, and writes the full trace (save_replay) only if all match.
// Input file: the trace header and action lines as in save_replay, then
//   DAILY <n>
//   <step> <money0> <money1> <inventory x N_PRODUCTS>   (n lines)
// usage: db_trace <list>   (each list line: <input> <output>); prints one status line per file
#include "source/world.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace dc10;

namespace {
std::string convert(const std::string& in_path, const std::string& out_path) {
    std::ifstream in(in_path);
    if (!in) return "cannot open";
    Config c;
    int count = 0;
    std::string tag, version, sha;
    in >> c.seed >> count >> tag;
    if (!in || count != 719 || tag != "CONFIG") return "bad header";
    in >> c.episode_steps >> c.board_size >> c.starting_money >> c.max_orders >> c.turns_per_day >> c.shed_capacity >>
        c.weed_chance >> c.shop_unlock_interval >> c.shop_sell_interval >> c.center_sell_interval >> c.hire_mult;
    in >> tag >> version >> sha;
    if (tag != "ENGINE" || version != OFFICIAL_VERSION || sha != OFFICIAL_SOURCE_SHA256) return "engine mismatch";
    std::vector<std::array<Action, 2>> turns(count);
    for (auto& turn : turns)
        for (auto& a : turn) {
            in >> a.n_units >> a.n_orders;
            if (!in || a.n_units < 1 || a.n_units > MAX_UNITS || a.n_orders < 0 || a.n_orders > 10) return "bad action";
            for (int u = 0; u < a.n_units; ++u) {
                int op, arg;
                in >> op >> arg >> a.units[u].n;
                a.units[u].op = uint8_t(op);
                a.units[u].arg = uint8_t(arg);
            }
            for (int k = 0; k < a.n_orders; ++k) {
                int op, item;
                in >> op >> item >> a.orders[k].n;
                a.orders[k].op = uint8_t(op);
                a.orders[k].item = uint8_t(item);
            }
            a.finalize();
        }
    int n = 0;
    in >> tag >> n;
    if (!in || tag != "DAILY" || n < 1) return "no daily snapshots";
    std::vector<std::array<double, 2 + N_PRODUCTS>> daily(n);
    std::vector<int> steps(n);
    for (int k = 0; k < n; ++k) {
        in >> steps[k];
        for (auto& v : daily[k]) in >> v;
    }
    if (!in) return "bad daily";
    Sim sim(c);
    int next = 0;
    for (int s = 0; s <= count && next < n; ++s) {
        while (next < n && steps[next] == s) {
            const auto& d = daily[next];
            for (int p = 0; p < 2; ++p)
                if (std::abs(sim.st.farms[p].money - d[p]) > 1e-6) return "money mismatch at step " + std::to_string(s);
            for (int i = 0; i < N_PRODUCTS; ++i)
                if (sim.st.market.inventory[i] != int(d[2 + i])) return "market mismatch at step " + std::to_string(s);
            ++next;
        }
        if (s < count) sim.step(turns[s][0], turns[s][1]);
    }
    if (next < n) return "snapshot after the last step";
    save_replay(out_path, c, turns);
    return "ok";
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: db_trace <list>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    std::string in_path, out_path;
    while (list >> in_path >> out_path) std::printf("%s %s\n", in_path.c_str(), convert(in_path, out_path).c_str());
    return 0;
}
