#pragma once
#include "biology.hpp"
#include "state.hpp"
#include <array>
#include <compare>

namespace kag::day_compiler {
constexpr int MAX_GROUPS = BOARD * BOARD;
constexpr int MAX_CROP_OPTIONS = 32;

struct CropPartition { std::array<int16_t, MAX_CROP_OPTIONS> counts{}; };
struct AnimalPartition { int16_t serve_today_count = 0, allow_escape_tonight_count = 0; };

// The complete network decision vector from design.pdf, sections 3.1–3.5.
// It contains no exact tiles, routes, purchases, hours or sale quantities.
struct DayIntent {
    int16_t new_wheat_count = 0, new_carrot_count = 0, new_melon_count = 0;
    int16_t new_tomato_count = 0, new_strawberry_count = 0;
    int16_t target_cows_next_dawn = 0, target_sheep_next_dawn = 0, target_geese_next_dawn = 0;
    bool buy_next_land_today = false;
    std::array<CropPartition, MAX_GROUPS> crops{};
    std::array<AnimalPartition, MAX_GROUPS> animals{};
    int new_crops(int product) const;
    int animal_target(int species) const;
};

struct CropState {
    int16_t product = 0, age = 0, held = 0, dry = 0, fertilizer_expiry = 0;
    int16_t decay_expiry = -32768, production_phase = 0;
    bool watered_today = false;
    auto operator<=>(const CropState&) const = default;
};
struct AnimalState {
    int16_t species = 0, age = 0, production_phase = 0, held = 0, unfed = 0, care_bank = 0, structure = 0;
    bool fed_today = false, cared_today = false, fertilizer_available = false;
    auto operator<=>(const AnimalState&) const = default;
};
struct CropProcedure {
    CropGoal goal{};
    int16_t water_actions = 0, fertilizer_actions = 0;
    int16_t maximum_count = 0;
};
struct CropGroup {
    CropState state{};
    int count = 0, option_count = 0;
    std::array<uint8_t, 100> cells{};
    std::array<CropProcedure, MAX_CROP_OPTIONS> options{};
};
struct AnimalGroup {
    AnimalState state{};
    int count = 0;
    std::array<uint8_t, 100> cells{};
};
// Derived from the observation. Group cells are not learned assignments.
struct IntentSchema {
    int crop_group_count = 0, animal_group_count = 0;
    std::array<CropGroup, MAX_GROUPS> crops{};
    std::array<AnimalGroup, MAX_GROUPS> animals{};
};

namespace detail {
struct BoundIntent {
    std::array<CropGoal, 100> crops{};
    std::array<bool, 100> serve{}, escape{};
    int new_crops[N_CROPS]{}, animal_target[3]{};
    bool buy_land = false;
};
}

enum class IntentError {
    None, NotDawn, InvalidCount, UnusedSlot, CropPartition, UnreachableGoal,
    AnimalPartition, ImpossibleEscape, AnimalTarget, NoLand
};
class IntentBinder {
public:
    IntentError describe(const Observation& observation, IntentSchema& schema);
    IntentError bind(const Observation& observation, const DayIntent& intent,
                     detail::BoundIntent& bound, IntentSchema* schema = nullptr, bool today_first = false);
private:
    Biology biology_;
};
}
