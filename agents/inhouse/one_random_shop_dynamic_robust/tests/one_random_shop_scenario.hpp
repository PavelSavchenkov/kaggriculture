#pragma once

#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace one_random_shop_scenario {

inline constexpr int FIRST_SHOP_UNLOCK_STEP = 71;
inline constexpr int NO_LATER_SHOP_INTERVAL = 1000;

inline void step(kag::Sim& sim, const kag::Action& first,
                 const kag::Action& second) {
    if (sim.st.step == FIRST_SHOP_UNLOCK_STEP) {
        if (sim.cfg.shop_unlock_interval != 3 || sim.st.n_shops != 0)
            std::abort();
        sim.cfg.shop_unlock_interval = NO_LATER_SHOP_INTERVAL;
    }
    sim.step(first, second);
    if (sim.st.day >= 3 && sim.st.n_shops != 1) std::abort();
}

inline void step_forced(kag::Sim& sim, const kag::Action& first,
                        const kag::Action& second, int shop) {
    step(sim, first, second);
    if (sim.st.day >= 3) sim.st.shops[0] = static_cast<uint8_t>(shop);
}

}  // namespace one_random_shop_scenario
