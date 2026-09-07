#pragma once
#include <array>
#include <cstdint>
#include "fast_game_engine/sim.hpp"

namespace kag::agents::bohann_opening_v1::detail {
inline constexpr uint32_t season_days = (uint32_t{1} << 30) - 1;

struct Cohort {
    uint8_t item = kag::WHEAT;
    int count = 1;
    int start_day = 0, end_day = 30;  // start inclusive; end exclusive
};

struct Service {
    uint32_t water = season_days, feed = season_days, care = season_days;
    uint32_t collect_fertilizer = season_days, harvest = season_days;
    uint32_t fertilize = 0;
};

struct BiologicalDay {
    int output[kag::N_PRODUCTS]{};
    int wheat = 0, fertilizer = 0, operations = 0, active = 0;
};

struct Biology {
    std::array<BiologicalDay, 30> days{};
    int seed_cost = 0, animal_cost = 0, structure_cost = 0;
    int disappearance_day = 30;
};

inline bool on(uint32_t mask, int day) { return (mask & (uint32_t{1} << day)) != 0; }

// Exact dated biology under explicit service assumptions. Travel, supply,
// capacity and cash feasibility belong to the estimator/compiler above this.
inline Biology biology(const Cohort& c, const Service& service) {
    if (c.count < 1 || c.start_day < 0 || c.start_day >= c.end_day || c.end_day > 30 ||
        !(kag::is_crop(c.item) || kag::is_animal(c.item))) std::abort();
    Biology result;
    const bool animal = kag::is_animal(c.item);
    const auto crop = animal ? kag::CropDef{} : kag::CROPS[c.item];
    const auto stock = animal ? kag::ANIMALS[c.item-kag::GOOSE] : kag::AnimalDef{};
    if (animal) {
        result.animal_cost = c.count * stock.cost;
        result.structure_cost = 0;  // Building consumes a worker action, no money.
    } else result.seed_cost = c.count * crop.seed;
    int held = animal || crop.ongoing ? 0 : 1;
    int dry = animal ? 0 : 1, care_bank = 0, fertilized_until = -1;
    int expiration = animal ? 30 : crop.ongoing ? 30 : c.start_day + crop.max_yield_day + 1;
    bool fertilizer_available = false;
    for (int day = c.start_day; day < c.end_day && day < expiration; ++day) {
        auto& d = result.days[day];
        const int age = day - c.start_day;
        d.active = c.count;
        if (age == 0) d.operations += (animal ? 2 : 1) * c.count; // place/build or plant
        const bool water = on(service.water, day), feed = on(service.feed, day), care = on(service.care, day);
        if (!animal) {
            if (on(service.fertilize, day)) {
                fertilized_until = day + 2;
                d.fertilizer += c.count; d.operations += c.count;
            }
            if (water) {
                d.operations += c.count;
                if (!crop.ongoing && age >= (crop.max_yield_day+1)/2 && age <= crop.max_yield_day)
                    held = std::min(crop.max_yield, held + (fertilized_until >= day ? 2 : 1));
            }
        } else {
            if (feed) { d.wheat += c.count; d.operations += c.count; }
            if (care) d.operations += c.count;
            if (fertilizer_available && on(service.collect_fertilizer, day)) {
                d.output[kag::FERTILIZER] += c.count; d.operations += c.count;
                fertilizer_available = false;
            }
        }
        if (held > 0 && on(service.harvest, day) && (animal || age >= crop.first_yield_day)) {
            d.output[animal ? stock.product : c.item] += c.count * held;
            d.operations += c.count; held = 0;
            if (!animal && !crop.ongoing) { result.disappearance_day = day; break; }
        }
        if (day == 29 || day+1 == c.end_day) { result.disappearance_day = day+1; break; }
        dry = (animal ? feed : water) ? 0 : dry+1;
        if (dry >= 2) { result.disappearance_day = day+1; break; }
        if (animal) {
            const int since = age+1-stock.first_yield_day;
            if (since >= 0 && since % stock.interval == 0) {
                held = std::min(stock.max_held, held+1+(feed ? care_bank : 0));
                care_bank = 0;
            }
            if (care && feed) ++care_bank;
            fertilizer_available = true;
        } else if (crop.ongoing) {
            const int since = age+1-crop.first_yield_day;
            if (since >= 0 && since % crop.interval == 0) {
                const int number = since/crop.interval + 1;
                if (number <= crop.max_yield) {
                    held = std::min(crop.max_yield, held + (water && fertilized_until >= day ? 2 : 1));
                    if (number == crop.max_yield) expiration = day+2;
                }
            }
        }
        result.disappearance_day = std::min(c.end_day, expiration);
    }
    return result;
}

inline Service productive_service(const Cohort& cohort, bool fertilize = false) {
    Service service;
    if (kag::is_crop(cohort.item)) {
        const auto& crop = kag::CROPS[cohort.item];
        const int last = std::min(cohort.end_day-1, cohort.start_day +
            (crop.ongoing ? crop.first_yield_day+(crop.max_yield-1)*crop.interval : crop.max_yield_day));
        // The default retires service after the last productive day. Decay-day
        // salvage at particular hours requires the intraday realization model.
        service.water &= (uint32_t{1} << (last+1))-1;
        service.harvest &= (uint32_t{1} << (last+1))-1;
        if (!crop.ongoing) {
            const int harvest = std::min(cohort.end_day-1, cohort.start_day+crop.max_yield_day);
            service.harvest = uint32_t{1} << harvest;
        }
        if (fertilize) {
            const int first = cohort.start_day + (crop.ongoing ? crop.first_yield_day-1 : (crop.max_yield_day+1)/2);
            for (int day = first; day <= last; day += 3) service.fertilize |= uint32_t{1} << day;
        }
    }
    return service;
}
}
