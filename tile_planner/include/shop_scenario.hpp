#pragma once
#include "course.hpp"

namespace placement {
// Evaluation intervention only. The compiler and both policies receive only
// the shops already revealed at this turn, never this scenario's future.
class ShopScenario {
    std::optional<kag::Sim> reference;
public:
    explicit ShopScenario(std::optional<uint64_t> seed) {
        if (seed) { kag::Config config; config.seed = *seed; config.weed_chance = 0; reference.emplace(config); }
    }
    void apply(kag::Sim& sim) const {
        if (!reference) return;
        sim.st.n_shops = reference->st.n_shops;
        std::copy_n(reference->st.shops, sim.st.n_shops, sim.st.shops);
    }
    void advance() {
        if (!reference) return;
        kag::Action pass; pass.n_units = 1; pass.finalize(); reference->step(pass, pass);
    }
};
inline void save_shops(std::ostream& output, const kag::Sim& sim, int turn) {
    output << turn << ' ' << sim.st.n_shops;
    for (int i = 0; i < sim.st.n_shops; ++i) output << ' ' << +sim.st.shops[i];
    output << '\n';
}
inline void load_shops(std::istream& input, kag::Sim& sim, int turn) {
    int recorded_turn = -1, count = -1; input >> recorded_turn >> count;
    if (!input || recorded_turn != turn || count < 0 || count > kag::MAX_SHOP_INSTANCES) throw std::runtime_error("invalid shop scenario record");
    sim.st.n_shops = count;
    for (int i = 0; i < count; ++i) {
        int shop = -1; input >> shop;
        if (!input || shop < 0 || shop >= kag::N_SHOPS) throw std::runtime_error("invalid shop scenario type");
        sim.st.shops[i] = shop;
    }
}
}
