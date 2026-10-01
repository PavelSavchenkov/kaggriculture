// Evaluation-only common random numbers for shops (copied from work/sep25_bc_weakness/tools, Sep 26).
// The engine draws each unlocked shop from the night RNG after the weed draws (one per empty tile on both
// farms), so any change in the farm layout changes every later shop and paired seeds stop being paired
// (after a day-10 decision change only 25-42% of games keep the same shops by day 24). With SHOP_CRN set in
// the environment, the harness replaces each newly unlocked shop by a draw that depends only on the seed and
// the shop's index: same distribution (uniform over the 8 types), independent of play. The engine is not
// changed; traces of such games must be replayed with SHOP_CRN set as well (tools/ledger3, shops_audit).
#pragma once
#include "fast_game_engine/pyrandom.hpp"
#include "fast_game_engine/sim.hpp"
#include <cstdlib>

inline bool shop_crn_enabled() {
    static const bool on = std::getenv("SHOP_CRN") != nullptr;
    return on;
}

// Call after every sim.step; `known` counts the shops already fixed (start at 0).
inline void shop_crn(kag::Sim& sim, int& known) {
    if (!shop_crn_enabled()) return;
    for (; known < sim.st.n_shops; ++known) {
        kag::PyRandom rng((sim.cfg.seed * 7919ull) ^ (0x5eedull << 32) ^ uint64_t(known));
        sim.st.shops[known] = uint8_t(rng.choice_index(kag::N_SHOPS));
    }
}
