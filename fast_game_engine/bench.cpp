// Throughput benchmark: replay a recorded episode as fast as possible.
#include "sim.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>

using namespace kag;
struct Turn { Action a[2]; };

// Older trace files have no CONFIG line. Peek for the token and rewind if it is
// absent, so both formats load.
static void read_config(std::istream& in, Config& cfg) {
    std::streampos pos = in.tellg();
    std::string tok;
    if (!(in >> tok)) return;
    if (tok != "CONFIG") { in.clear(); in.seekg(pos); return; }
    in >> cfg.episode_steps >> cfg.board_size >> cfg.starting_money
       >> cfg.max_orders >> cfg.turns_per_day >> cfg.shed_capacity
       >> cfg.weed_chance >> cfg.shop_unlock_interval
       >> cfg.shop_sell_interval >> cfg.center_sell_interval >> cfg.hire_mult;
}

static void skip_engine(std::istream& in) {
    std::streampos pos = in.tellg();
    std::string tok;
    if (!(in >> tok)) return;
    if (tok != "ENGINE") { in.clear(); in.seekg(pos); return; }
    std::string version, source_hash;
    in >> version >> source_hash;
}

static void load(const std::string& path, uint64_t& seed, Config& cfg,
                 std::vector<Turn>& turns) {
    std::ifstream in(path);
    int n; in >> seed >> n;
    read_config(in, cfg);
    skip_engine(in);
    turns.resize(n);
    for (int t = 0; t < n; ++t)
        for (int p = 0; p < 2; ++p) {
            int nu, no; in >> nu >> no;
            Action& a = turns[t].a[p];
            a.n_units = std::min(nu, MAX_UNITS);
            for (int i = 0; i < nu; ++i) {
                int op, arg, cnt; in >> op >> arg >> cnt;
                if (i < MAX_UNITS) { a.units[i] = { (uint8_t)op, (uint8_t)arg, cnt }; }
            }
            a.n_orders = std::min(no, 16);
            for (int i = 0; i < no; ++i) {
                int op, item, cnt; in >> op >> item >> cnt;
                if (i < 16) a.orders[i] = { (uint8_t)op, (uint8_t)item, cnt };
            }
            a.finalize();
        }
}

int main(int argc, char** argv) {
    uint64_t seed; std::vector<Turn> turns;
    Config base;                      // overwritten by the file's CONFIG line
    load(argc > 1 ? argv[1] : "../data/replay_70117.txt", seed, base, turns);
    int reps = argc > 2 ? std::atoi(argv[2]) : 2000;
    int rounds = argc > 3 ? std::atoi(argv[3]) : 1;

    double sink = 0;
#ifdef KAG_PROFILE
    Sim::PhaseProfile profile;
#endif
    std::vector<double> timings;
    timings.reserve(rounds);
    for (int round = 0; round < rounds; ++round) {
        auto t0 = std::chrono::steady_clock::now();
        for (int r = 0; r < reps; ++r) {
            Config cfg = base;
            cfg.seed = seed + r + static_cast<uint64_t>(round) * reps;
            Sim sim(cfg);
            for (auto& tn : turns) sim.step(tn.a[0], tn.a[1]);
            sink += sim.st.farms[0].money;
#ifdef KAG_PROFILE
            profile.units += sim.profile.units;
            profile.market += sim.profile.market;
            profile.town += sim.profile.town;
            profile.decay += sim.profile.decay;
            profile.end_day += sim.profile.end_day;
#endif
        }
        timings.push_back(
            std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    }
    std::sort(timings.begin(), timings.end());
    const double median = timings[timings.size() / 2];
    const double best = timings.front();
    std::printf("%d episodes x %d: median %.0f eps/s (%.3f ms/ep), best %.0f eps/s [sink %.0f]\n",
                reps, rounds, reps / median, median / reps * 1000, reps / best, sink);
#ifdef KAG_PROFILE
    const double total = profile.units + profile.market + profile.town +
                         profile.decay + profile.end_day;
    std::printf("profile cycles: units %.1f%%, market %.1f%%, town %.1f%%, decay %.1f%%, end-day %.1f%%\n",
                100 * profile.units / total, 100 * profile.market / total,
                100 * profile.town / total, 100 * profile.decay / total,
                100 * profile.end_day / total);
#endif
    return 0;
}
