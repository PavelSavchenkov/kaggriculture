#pragma once
#include "agents/common/api/observation.hpp"
#include <array>

namespace kag::day_compiler {
enum class CropMode : uint8_t { Retire, Lean, Full, Yield };
struct CropGoal {
    CropMode mode = CropMode::Lean;
    uint8_t harvest_age = 0;
    uint8_t min_yield = 0;
};
struct Calendar {
    uint32_t water_days = 0, fertilizer_days = 0;
    uint8_t waters = 0, fertilizers = 0, yield = 0;
};
struct Calendars {
    int count = 0;
    Calendar plans[32]{};
};
bool zero_value_care(const Tile& tile, int day, int last_day = 29);
int next_animal_production(const Tile& tile, int after_day);

class Biology {
public:
    Calendars one_shot(const Tile& tile, int day, int harvest_age, int minimum_yield, int last_day = 29);
    Calendars ongoing(const Tile& tile, int day, bool full, int last_day = 29);
    uint64_t cache_hits = 0, cache_misses = 0;
private:
    struct Entry { uint64_t key = 0; Calendars result; };
    std::array<Entry, 512> cache_{};
};
}
