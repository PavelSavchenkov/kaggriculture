#pragma once
// DayIntent training targets and fixed-value masks (designs/day_intent.md, "Fixed values").
// A mask of 1 means the field or option is fixed: no loss, no metric, and decoding
// never chooses another value. Masks are decided from the dawn schema alone.
#include "source/features.hpp"

namespace bcopus {

// Whole-farm fields: 5 new crops, 3 new animals, 3 reserves, buy land.
constexpr int GLOBAL_FIELDS = 12;
constexpr int COUNT_CLASSES = 101;  // integer counts 0-100 (designs/day_intent.md)

// Crop group targets: size, 9 one-shot options, retain, clear, fertilize, harvest.
constexpr int CROP_TARGETS = 14;
// Crop group masks: 9 option masks, retain, clear, abandon, fertilize, harvest.
constexpr int CROP_MASKS = 14;
// Animal group targets: size, feed, care, collect. Masks: feed, care, collect.
constexpr int ANIMAL_TARGETS = 4;
constexpr int ANIMAL_MASKS = 3;

inline void global_masks(const Schema& s, uint8_t mask[GLOBAL_FIELDS]) {
    const bool terminal = s.day == LAST_DAY;
    for (int f = 0; f < 11; ++f) mask[f] = terminal;  // day 29: new counts 0, reserve = dawn stock
    mask[11] = 0;
}

inline void global_targets(const DayIntent& in, int16_t out[GLOBAL_FIELDS]) {
    for (int c = 0; c < N_CROPS; ++c) out[c] = in.new_crop[c];
    for (int a = 0; a < N_ANIMALS; ++a) out[5 + a] = in.new_animal[a], out[8 + a] = in.reserve[a];
    out[11] = in.buy_land;
}

// Masks for one crop group. `size` is the group size (new groups: the new count).
inline void crop_masks(const Schema& s, const CropGroup& g, int size, uint8_t mask[CROP_MASKS]) {
    std::fill_n(mask, CROP_MASKS, 1);
    if (size == 0) return;  // an empty new group: every field is zero
    if (!CROPS[g.crop].ongoing) {
        for (int o = 0; o < OPTIONS; ++o) mask[o] = option_fixed(g, s.day, o);
        return;
    }
    if (g.fresh) return;  // new ongoing groups have no output fields
    const bool terminal = s.day == LAST_DAY;
    // Partition retain / clear / abandon; unused on day 29.
    mask[9] = terminal || !can_survive_tonight(g, s.day);
    mask[10] = terminal;
    mask[11] = terminal || !can_die_tonight(g);
    mask[12] = ongoing_fertilize_fixed(g, s.day);
    mask[13] = g.yield == 0;
}

inline void crop_targets(const DayIntent& in, int i, int size, int16_t out[CROP_TARGETS]) {
    out[0] = int16_t(size);
    for (int o = 0; o < OPTIONS; ++o) out[1 + o] = in.options[i][o];
    out[10] = in.retain[i];
    out[11] = in.clear[i];
    out[12] = in.fertilize[i];
    out[13] = in.harvest[i];
}

inline void animal_masks(const Schema& s, const AnimalGroup& g, int size, uint8_t mask[ANIMAL_MASKS]) {
    const bool empty = size == 0, terminal = s.day == LAST_DAY;
    mask[0] = empty || terminal;
    mask[1] = mask[0] || care_fixed(g, s.day);
    mask[2] = empty || g.held == 0;
}

inline void animal_targets(const DayIntent& in, int i, int size, int16_t out[ANIMAL_TARGETS]) {
    out[0] = int16_t(size);
    out[1] = in.feed[i];
    out[2] = in.care[i];
    out[3] = in.collect[i];
}
}
