#include "intent.hpp"
#include <map>
#include <tuple>

namespace dc10 {

Schema describe(const agent::AgentObservation& dawn) {
    Schema s;
    s.day = dawn.day;
    using CropKey = std::tuple<int, int, int, int, int>;
    using AnimalKey = std::tuple<int, int, int, int, int>;
    std::map<CropKey, CropGroup> crops;
    std::map<AnimalKey, AnimalGroup> animals;
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        const Tile& t = tile_at(dawn, cell);
        s.open_sites += t.kind != T_PLANT && t.kind != T_LOCKED && !t.has_animal;
        s.land_sites += t.kind == T_LOCKED && quadrant_of(cell % BOARD, cell / BOARD, BOARD) == dawn.self().n_quadrants;
        if (t.kind == T_PLANT) {
            const int fert = fert_days_of(t, s.day);
            auto& g = crops[{t.what, s.day - t.planted_day, t.yield_units, t.consecutive_dry, fert}];
            g.crop = t.what;
            g.age = int16_t(s.day - t.planted_day);
            g.yield = t.yield_units;
            g.dry = t.consecutive_dry;
            g.fert_days = int8_t(fert);
            g.decaying = CROPS[t.what].ongoing && t.max_lifespan_step >= 0 &&
                         t.max_lifespan_step < (s.day + 1) * HOURS;
            g.cells[g.size++] = uint8_t(cell);
        }
        if (t.has_animal) {
            const int species = t.what - GOOSE;
            auto& g = animals[{species, s.day - t.planted_day, t.yield_units, t.consecutive_dry,
                               t.pending_care_bonus}];
            g.species = uint8_t(species);
            g.age = int16_t(s.day - t.planted_day);
            g.held = t.yield_units;
            g.unfed = t.consecutive_dry;
            g.bonus = t.pending_care_bonus;
            g.cells[g.size++] = uint8_t(cell);
        }
    }
    for (auto& [key, g] : crops) s.crops[s.n_crops++] = g;
    for (int crop = 0; crop < N_CROPS; ++crop) {
        CropGroup g;
        g.crop = uint8_t(crop);
        g.yield = CROPS[crop].ongoing ? 0 : 1;
        g.dry = 1;
        g.fresh = true;
        s.new_crop_group[crop] = s.n_crops;
        s.crops[s.n_crops++] = g;
    }
    for (auto& [key, g] : animals) s.animals[s.n_animals++] = g;
    for (int a = 0; a < N_ANIMALS; ++a) {
        AnimalGroup g;
        g.species = uint8_t(a);
        g.fresh = true;
        s.new_animal_group[a] = s.n_animals;
        s.animals[s.n_animals++] = g;
        s.unplaced_dawn[a] = dawn.own.shed[GOOSE + a];
    }
    return s;
}

void size_new_groups(Schema& s, const DayIntent& intent) {
    for (int crop = 0; crop < N_CROPS; ++crop) s.crops[s.new_crop_group[crop]].size = intent.new_crop[crop];
    for (int a = 0; a < N_ANIMALS; ++a) s.animals[s.new_animal_group[a]].size = intent.new_animal[a];
}

namespace {
bool water_adds_yield(const CropGroup& g) {
    const auto& c = CROPS[g.crop];
    return g.age >= (c.max_yield_day + 1) / 2 && g.age <= c.max_yield_day && g.yield < c.max_yield;
}

bool water_useful(const CropGroup& g, int day, bool harvest) {
    if (harvest) return water_adds_yield(g);
    if (day == LAST_DAY) return false;
    return g.age <= CROPS[g.crop].max_yield_day;  // otherwise the crop decays to a weed today
}

bool fertilize_useful(const CropGroup& g, int day, bool water, bool harvest) {
    const auto& c = CROPS[g.crop];
    if (g.yield + 1 >= c.max_yield) return false;  // doubling can no longer add a unit
    if (harvest) return water && g.fert_days == 0 && water_adds_yield(g);
    if (day == LAST_DAY || g.age > c.max_yield_day) return false;
    const int w0 = (c.max_yield_day + 1) / 2;
    for (int k = std::max<int>(g.fert_days, water ? 0 : 1); k <= 2 && day + k <= LAST_DAY; ++k)
        if (g.age + k >= w0 && g.age + k <= c.max_yield_day) return true;
    return false;
}
}

bool option_fixed(const CropGroup& g, int day, int o) {
    if (o == CLEAR) return g.fresh || day == LAST_DAY;
    if (g.fresh && !has_water(o)) return true;
    if (has_harvest(o) && g.age < CROPS[g.crop].first_yield_day) return true;
    if (has_water(o) && !water_useful(g, day, has_harvest(o))) return true;
    if (has_fertilize(o) && !fertilize_useful(g, day, has_water(o), has_harvest(o))) return true;
    return false;
}

bool ongoing_fertilize_fixed(const CropGroup& g, int day) {
    const auto& c = CROPS[g.crop];
    for (int k = g.fert_days; k <= 2; ++k) {
        const int night = day + k;
        if (night > LAST_DAY - 1) break;
        const int since = g.age + k + 1 - c.first_yield_day;
        if (since >= 0 && since % c.interval == 0 && since / c.interval < c.max_yield) return false;
    }
    return true;
}

bool care_fixed(const AnimalGroup& g, int day) {
    const auto& a = ANIMALS[g.species];
    // Today's care banks for the first production after tonight's.
    for (int night = day + 1; night <= LAST_DAY - 1; ++night) {
        const int since = g.age + (night - day) + 1 - a.first_yield_day;
        if (since >= 0 && since % a.interval == 0) return false;
    }
    return true;
}

bool can_survive_tonight(const CropGroup& g, int day) { return day < LAST_DAY && !g.decaying; }
bool can_die_tonight(const CropGroup& g) { return g.dry >= 1 || g.decaying; }

int free_sites(const Schema& s, const DayIntent& in) {
    int sites = s.open_sites + (in.buy_land ? s.land_sites : 0);
    for (int i = 0; i < s.n_crops; ++i) {
        if (s.crops[i].fresh) continue;
        if (CROPS[s.crops[i].crop].ongoing) sites += in.clear[i];
        else if (turns_weed_today(s.crops[i])) sites += s.crops[i].size;
        else
            for (int o = 0; o < OPTIONS; ++o) sites += (o == CLEAR || has_harvest(o)) ? in.options[i][o] : 0;
    }
    return sites;
}

int new_entities(const DayIntent& in) {
    int n = 0;
    for (int c = 0; c < N_CROPS; ++c) n += in.new_crop[c];
    for (int a = 0; a < N_ANIMALS; ++a) n += in.new_animal[a];
    return n;
}

std::string validate(const Schema& s, const DayIntent& in) {
    const bool terminal = s.day == LAST_DAY;
    for (int a = 0; a < N_ANIMALS; ++a) {
        if (in.new_animal[a] < 0 || in.reserve[a] < 0) return "negative animal count";
        if (terminal && (in.new_animal[a] != 0 || in.reserve[a] != s.unplaced_dawn[a]))
            return "day-29 animal counts are fixed";
        if (in.new_animal[a] + in.reserve[a] < s.unplaced_dawn[a]) return "new + reserve below dawn stock";
    }
    for (int c = 0; c < N_CROPS; ++c) {
        if (in.new_crop[c] < 0) return "negative new crop count";
        if (terminal && in.new_crop[c] != 0) return "day-29 new crops are fixed to zero";
    }
    for (int i = 0; i < s.n_crops; ++i) {
        const auto& g = s.crops[i];
        const int size = g.fresh ? in.new_crop[g.crop] : g.size;
        if (!CROPS[g.crop].ongoing) {
            int sum = 0;
            for (int o = 0; o < OPTIONS; ++o) {
                const int n = in.options[i][o];
                if (n < 0) return "negative one-shot option";
                if (n > 0 && (size == 0 || option_fixed(g, s.day, o))) return "fixed one-shot option used";
                sum += n;
            }
            if (sum != size) return "one-shot partition does not sum to group size";
            continue;
        }
        if (g.fresh) continue;
        const int r = in.retain[i], cl = in.clear[i], f = in.fertilize[i], h = in.harvest[i];
        if (r < 0 || cl < 0 || f < 0 || h < 0 || h > size) return "ongoing count out of range";
        if (h > 0 && g.yield == 0) return "harvest without held product";
        if (terminal) {
            if (r || cl || f) return "day-29 ongoing retain, clear and fertilize are fixed to zero";
            continue;
        }
        if (r + cl > size) return "retain + clear exceeds group size";
        if (r > 0 && !can_survive_tonight(g, s.day)) return "retained crop cannot survive tonight";
        if (!can_die_tonight(g) && r + cl != size) return "abandon impossible for this group";
        if (f > r) return "fertilize exceeds retain";
        if (f > 0 && ongoing_fertilize_fixed(g, s.day)) return "fixed ongoing fertilize used";
    }
    for (int i = 0; i < s.n_animals; ++i) {
        const auto& g = s.animals[i];
        const int size = g.fresh ? in.new_animal[g.species] : g.size;
        const int f = in.feed[i], c = in.care[i], k = in.collect[i];
        if (f < 0 || c < 0 || k < 0 || f > size || c > f || k > size) return "animal count out of range";
        if (terminal && f) return "day-29 feed is fixed to zero";
        if (c > 0 && care_fixed(g, s.day)) return "fixed care used";
        if (k > 0 && g.held == 0) return "collect without held product";
    }
    if (new_entities(in) > free_sites(s, in))
        return "new crops and animals exceed free sites (" + std::to_string(new_entities(in)) + " > " +
               std::to_string(free_sites(s, in)) + ")";
    return {};
}
}
