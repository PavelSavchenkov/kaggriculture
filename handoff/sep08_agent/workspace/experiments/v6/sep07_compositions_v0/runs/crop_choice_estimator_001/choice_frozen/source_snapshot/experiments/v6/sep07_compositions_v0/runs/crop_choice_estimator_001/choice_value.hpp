#pragma once
#include "financial_plans/plans.hpp"

namespace compositions::crop_choice {
// Same fixed stratification as include/sampled_animal_value.hpp. Only already
// revealed shops are copied. The fixed sample index is not the game seed.
inline uint64_t sample_random(uint64_t& state) {
    uint64_t x = (state += 0x9e3779b97f4a7c15ULL);
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

inline std::array<int,8> sample_shops(const kag::agent::AgentObservation& o, int sample) {
    std::array<int,8> shops{};
    for (int reveal = 0; reveal < 8; ++reveal) {
        if (reveal < o.n_shops) { shops[reveal] = o.shops[reveal]; continue; }
        std::array<int,8> permutation{0,1,2,3,4,5,6,7};
        uint64_t state = uint64_t(sample / 8 + 1) * 0xd1b54a32d192ed03ULL
                       ^ uint64_t(reveal + 1) * 0x94d049bb133111ebULL;
        for (int i = 7; i > 0; --i) std::swap(permutation[i], permutation[sample_random(state) % (i + 1)]);
        shops[reveal] = permutation[sample % 8];
    }
    return shops;
}

inline Days demand_for(const std::array<int,8>& shops, int first_day) {
    Days result{};
    for (int day = first_day; day < 30; ++day) for (int product = 0; product < kag::N_PRODUCTS; ++product) {
        result[day][product] = product == kag::FERTILIZER ? 0 : 1;
        for (int reveal = 0; reveal < std::min(8, day / 3); ++reveal)
            if (kag::SHOP_MASK[shops[reveal]] & (1u << product)) result[day][product] += 6 * kag::SHOP_MULT[shops[reveal]];
    }
    return result;
}

inline bool berry_for(const std::array<int,8>& shops) {
    int demand = 0;
    for (int reveal = 0; reveal < 6; ++reveal)
        if (kag::SHOP_MASK[shops[reveal]] & (1u << kag::STRAWBERRY)) demand += kag::SHOP_MULT[shops[reveal]];
    return demand >= 4;
}

inline std::array<Value,2> estimate_choice(const kag::agent::AgentObservation& o, int model) {
    if (o.day != 12 || o.hour != 0 || model < 0 || model >= 16) std::abort();
    const auto rival = rival_flows(o, model / 4, bool(model & 2), bool(model & 1));
    std::array<Value,2> result{};
    for (int sample = 0; sample < 64; ++sample) {
        const auto shops = sample_shops(o, sample);
        const auto demand = demand_for(shops, o.day);
        const int berry = berry_for(shops);
        for (int family = 0; family < 2; ++family) {
            const auto next = value(o, financial_plans[2 * family + berry], rival, demand);
            result[family].own += next.own / 64;
            result[family].rival += next.rival / 64;
            for (int product = 0; product < kag::N_PRODUCTS; ++product) {
                result[family].own_product[product] += next.own_product[product] / 64;
                result[family].rival_product[product] += next.rival_product[product] / 64;
            }
        }
    }
    return result;
}
}
