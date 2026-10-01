#pragma once
// DayIntent from designs/day_intent.md: groups, fields, fixed values, validation.
#include "world.hpp"
#include <array>
#include <string>

namespace dc10 {

// One-shot options 0-7 are bit sets of water (4), fertilize (2), harvest (1); 8 is clear.
constexpr int OPTIONS = 9;
constexpr int CLEAR = 8;
inline bool has_water(int o) { return o < CLEAR && (o & 4); }
inline bool has_fertilize(int o) { return o < CLEAR && (o & 2); }
inline bool has_harvest(int o) { return o < CLEAR && (o & 1); }

constexpr int MAX_GROUPS = 110;

struct CropGroup {
    uint8_t crop = 0;
    int16_t age = 0;
    int8_t yield = 0;      // one-shot harvest yield or ongoing held product
    int8_t dry = 0;
    int8_t fert_days = 0;  // remaining fertilizer days, starting today
    bool decaying = false; // ongoing crop past its last production: becomes a weed today
    bool fresh = false;    // today's new group
    int size = 0;          // members; for a new group, set from new_crop
    std::array<uint8_t, BOARD * BOARD> cells{};
};

struct AnimalGroup {
    uint8_t species = 0;   // 0 goose, 1 cow, 2 sheep
    int16_t age = 0;
    int8_t held = 0;
    int8_t unfed = 0;
    int8_t bonus = 0;
    bool fresh = false;
    int size = 0;
    std::array<uint8_t, BOARD * BOARD> cells{};
};

// Groups and dawn facts derived deterministically from the dawn observation.
struct Schema {
    int day = 0;
    int n_crops = 0, n_animals = 0;
    std::array<CropGroup, MAX_GROUPS> crops{};
    std::array<AnimalGroup, MAX_GROUPS> animals{};
    int unplaced_dawn[N_ANIMALS]{};
    int new_crop_group[N_CROPS]{};      // index of each new crop group
    int new_animal_group[N_ANIMALS]{};  // index of each new animal group
    int open_sites = 0;  // unlocked tiles without a plant or an animal (empty, weed, empty housing)
    int land_sites = 0;  // locked tiles of the next quadrant (usable if land is bought today)
};

struct DayIntent {
    int16_t new_crop[N_CROPS]{};
    int16_t new_animal[N_ANIMALS]{};
    int16_t reserve[N_ANIMALS]{};
    bool buy_land = false;
    std::array<std::array<int16_t, OPTIONS>, MAX_GROUPS> options{};  // one-shot groups
    std::array<int16_t, MAX_GROUPS> retain{}, clear{}, fertilize{}, harvest{};  // ongoing groups
    std::array<int16_t, MAX_GROUPS> feed{}, care{}, collect{};  // animal groups
    // Decoder hint for budget trims: new-entity types (crops 0-4, animals 5-7) in the order the
    // model gives up units most easily per dollar; trim_next is the compiler's cursor.
    std::array<int8_t, 32> trim_order{};
    int8_t trim_count = 0, trim_next = 0;
    // Decoder preferences for budget revisions: log-probability lost by removing the k-th unit
    // (index k-1) of each new-entity type (crops 0-4, animals 5-7).
    std::array<std::array<float, 32>, 8> drop_loss{};
    bool has_drop_loss = false;
};

inline int fert_days_of(const Tile& t, int day) {
    return std::clamp(t.fertilized_until_day - day + 1, 0, 3);
}

Schema describe(const agent::AgentObservation& dawn);
// Sizes of new groups follow the intent's new counts.
void size_new_groups(Schema& schema, const DayIntent& intent);

// Fixed-value rules. true means the option or count is fixed to zero.
bool option_fixed(const CropGroup& g, int day, int option);
bool ongoing_fertilize_fixed(const CropGroup& g, int day);
bool care_fixed(const AnimalGroup& g, int day);
bool can_survive_tonight(const CropGroup& g, int day);
bool can_die_tonight(const CropGroup& g);

// A one-shot crop past its max-yield day loses a unit every two hours from hour 0 and
// is a weed by hour 10 unless harvested or cleared: its tile is free today either way.
inline bool turns_weed_today(const CropGroup& g) { return !CROPS[g.crop].ongoing && g.age > CROPS[g.crop].max_yield_day; }

// Tiles where today's new crops and animals can go: open sites, tiles cleared today
// (one-shot harvests and clears, one-shot crops that turn into weeds, ongoing clears)
// and land bought today.
int free_sites(const Schema& schema, const DayIntent& intent);
int new_entities(const DayIntent& intent);

// Empty when the intent satisfies every rule of designs/day_intent.md that can be
// checked from the schema alone, including site capacity.
std::string validate(const Schema& schema, const DayIntent& intent);
}
