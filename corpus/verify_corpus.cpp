// Verify .kagz episodes against the fast engine, and time re-simulation.
//
//   verify_corpus data/*.kagz
//
// Loads each container, replays its action stream through kag::Sim and compares
// the canonical parity hash of every state. A pass means the compact form
// regenerates the recorded episode exactly.
#include <chrono>
#include <cstdio>
#include <vector>

#include "corpus/episode.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s episode.kagz [...]\n", argv[0]);
        return 2;
    }
    int failed = 0, states = 0;
    double load_ms = 0, sim_ms = 0, replay_ms = 0;
    using clock = std::chrono::steady_clock;
    for (int i = 1; i < argc; ++i) {
        corpus::Episode ep;
        auto t0 = clock::now();
        if (!corpus::load(argv[i], ep)) { ++failed; continue; }
        auto t1 = clock::now();
        int bad_step = -1;
        bool ok = corpus::verify(ep, &bad_step);
        auto t2 = clock::now();
        // Same replay without hashing: this is what the mining layer pays.
        kag::Sim plain(ep.config);
        auto t3 = clock::now();
        for (int t = 0; t + 1 < ep.n_steps; ++t)
            plain.step(ep.actions[size_t(t) + 1][0], ep.actions[size_t(t) + 1][1]);
        replay_ms += std::chrono::duration<double, std::milli>(clock::now() - t3).count();
        load_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        sim_ms += std::chrono::duration<double, std::milli>(t2 - t1).count();
        states += ep.n_steps;
        if (!ok) ++failed;
        std::printf("%s %-10llu %d states  seed=%-11llu rewards=(%.0f, %.0f)  %s vs %s",
                    ok ? "PASS" : "FAIL", (unsigned long long)ep.episode_id, ep.n_steps,
                    (unsigned long long)ep.seed, ep.rewards[0], ep.rewards[1],
                    ep.teams[0].c_str(), ep.teams[1].c_str());
        if (!ok && bad_step >= 0) std::printf("  first mismatch at step %d", bad_step);
        std::printf("\n");
    }
    std::printf("\n%d episodes, %d states, %d failed\n", argc - 1, states, failed);
    int n = argc - 1;
    std::printf("load        %7.1f ms total  %6.3f ms/episode\n", load_ms, load_ms / n);
    std::printf("verify      %7.1f ms total  %6.3f ms/episode  (parity hash at every state)\n",
                sim_ms, sim_ms / n);
    std::printf("replay      %7.1f ms total  %6.3f ms/episode  (no hashing - the mining path)\n",
                replay_ms, replay_ms / n);
    return failed ? 1 : 0;
}
