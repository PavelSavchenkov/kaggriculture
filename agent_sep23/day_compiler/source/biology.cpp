#include "biology.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <tuple>

namespace kag::day_compiler {
int next_animal_production(const Tile& t, int after_day) {
    const auto& a = ANIMALS[t.what - GOOSE];
    int next = t.planted_day + a.first_yield_day;
    if (next <= after_day) next += (1 + (after_day - next) / a.interval) * a.interval;
    return next;
}
bool zero_value_care(const Tile& t, int day, int last_day) {
    if (!t.has_animal) return true;
    const auto& a = ANIMALS[t.what - GOOSE];
    int next = next_animal_production(t, day);
    int bank = t.pending_care_bonus;
    if (next == day + 1) { next += a.interval; bank = 0; }
    return next > last_day || bank >= a.max_held - 1;
}
namespace {
constexpr int STATES = 2 * 4 * 7 * 16;
struct Label {
    uint32_t water = 0, fertilizer = 0;
    uint8_t cost = 255;
};
int index(int dry, int active, int yield, int fertilizer) {
    return (((dry * 4 + active + 1) * 7 + yield) * 16 + fertilizer);
}
void add(Calendars& out, Calendar a) {
    // Preserve different current-day service choices: they have different routes.
    const auto today = [](Calendar b) { return (b.water_days & 1) | ((b.fertilizer_days & 1) << 1); };
    for (int j = 0; j < out.count; ++j) {
        const auto b = out.plans[j];
        if (today(a) != today(b)) continue;
        if (b.waters <= a.waters && b.fertilizers <= a.fertilizers && b.yield >= a.yield) return;
    }
    for (int j = 0; j < out.count;) {
        const auto b = out.plans[j];
        if (today(a) == today(b) && a.waters <= b.waters && a.fertilizers <= b.fertilizers && a.yield >= b.yield)
            out.plans[j] = out.plans[--out.count];
        else ++j;
    }
    if (out.count < 32) out.plans[out.count++] = a;
}
Calendars solve(const Tile& tile, int day, int horizon, int minimum, bool full, bool ongoing) {
    Calendars out;
    if (horizon < 0 || horizon > 29 || minimum > 6) return out;
    const int age = day - tile.planted_day, p = tile.what;
    const auto& crop = CROPS[p];
    for (int first = 0; first < 4; ++first) {
        std::array<Label, STATES> current, next;
        int initial_dry = std::clamp<int>(tile.consecutive_dry, 0, 1);
        current[index(initial_dry, std::clamp<int>(tile.fertilized_until_day - day, -1, 2),
                      ongoing ? 0 : tile.yield_units, 0)] = {0, 0, 0};
        for (int d = 0; d <= horizon; ++d) {
            next.fill(Label{});
            for (int dry = 0; dry < 2; ++dry) for (int active = -1; active < 3; ++active)
            for (int y = 0; y <= (ongoing ? 0 : crop.max_yield); ++y) for (int used = 0; used < 16; ++used) {
                const auto label = current[index(dry, active, y, used)];
                if (label.cost == 255) continue;
                for (int action = 0; action < 4; ++action) {
                    if (d == 0 && action != first) continue;
                    const int w = action & 1, f = action >> 1;
                    if (used + f >= 16 || (d == 0 && ((w && tile.watered_today) || (f && active == 2)))) continue;
                    const int left = f ? 2 : active;
                    const int next_dry = (w || (d == 0 && tile.watered_today)) ? 0 : dry + 1;
                    int yield = y;
                    if (ongoing) {
                        const int production_age = age + d + 1;
                        const int last_production = crop.first_yield_day + crop.interval * (crop.max_yield - 1);
                        const bool productive = production_age >= crop.first_yield_day && production_age <= last_production &&
                            (production_age - crop.first_yield_day) % crop.interval == 0;
                        if (full && productive && !(left >= 0 && (w || (d == 0 && tile.watered_today)))) continue;
                    } else if (w && age + d >= (crop.max_yield_day + 1) / 2 && age + d <= crop.max_yield_day)
                        yield = std::min(crop.max_yield, yield + 1 + (left >= 0));
                    if ((ongoing || d < horizon) && next_dry >= 2) continue;
                    const int nd = (d == horizon && !ongoing) ? 0 : next_dry;
                    auto& target = next[index(nd, std::max(-1, left - 1), yield, used + f)];
                    const uint8_t cost = label.cost + w;
                    const uint32_t wm = label.water | (uint32_t(w) << d), fm = label.fertilizer | (uint32_t(f) << d);
                    // Stable late-service tie break saves unnecessary early commitments.
                    if (cost < target.cost || (cost == target.cost && std::pair{wm, fm} > std::pair{target.water, target.fertilizer}))
                        target = {wm, fm, cost};
                }
            }
            current.swap(next);
        }
        for (int dry = 0; dry < 2; ++dry) for (int active = -1; active < 3; ++active)
        for (int y = minimum; y <= (ongoing ? 0 : crop.max_yield); ++y) for (int used = 0; used < 16; ++used) {
            const auto label = current[index(dry, active, y, used)];
            if (label.cost != 255) add(out, {label.water, label.fertilizer, label.cost, uint8_t(used), uint8_t(y)});
        }
    }
    std::sort(out.plans, out.plans + out.count, [](Calendar a, Calendar b) {
        return std::tuple(a.waters + a.fertilizers * 2, a.fertilizers, a.water_days & 1, a.fertilizer_days & 1, a.water_days, a.fertilizer_days) <
               std::tuple(b.waters + b.fertilizers * 2, b.fertilizers, b.water_days & 1, b.fertilizer_days & 1, b.water_days, b.fertilizer_days);
    });
    return out;
}
uint64_t key(const Tile& t, int day, int horizon, int minimum, int mode) {
    uint64_t k = t.what;
    for (int value : {day - t.planted_day, int(t.yield_units), int(t.consecutive_dry),
                      std::clamp<int>(t.fertilized_until_day - day, -1, 2) + 1, int(t.watered_today), horizon, minimum, mode})
        k = k * 37 + value + 1;
    return k + 1;
}
}
Calendars Biology::one_shot(const Tile& t, int day, int target, int minimum, int last_day) {
    if (t.kind != T_PLANT || !is_crop(t.what) || CROPS[t.what].ongoing) return {};
    const int horizon = target - (day - t.planted_day);
    // Decay starts after the first worker phase on max_yield_day + 1.
    // Harvesting then is legal; the route enforces the quantity's hour deadline.
    if (target < CROPS[t.what].first_yield_day || target > CROPS[t.what].max_yield_day + 1 || day + horizon > last_day) return {};
    const uint64_t k = key(t, day, horizon, minimum, 0);
    auto& entry = cache_[(k * 11400714819323198485ull) >> 55];
    if (entry.key == k) { ++cache_hits; return entry.result; }
    ++cache_misses; entry = {k, solve(t, day, horizon, minimum, false, false)}; return entry.result;
}
Calendars Biology::ongoing(const Tile& t, int day, bool full, int last_day) {
    if (t.kind != T_PLANT || !is_crop(t.what) || !CROPS[t.what].ongoing) return {};
    const auto& crop = CROPS[t.what];
    const int last_production = crop.first_yield_day + crop.interval * (crop.max_yield - 1);
    const int horizon = std::min(last_day - day, last_production - (day - t.planted_day)) - 1;
    if (horizon < 0) { Calendars result; result.count = 1; return result; }
    const uint64_t k = key(t, day, horizon, 0, 1 + full);
    auto& entry = cache_[(k * 11400714819323198485ull) >> 55];
    if (entry.key == k) { ++cache_hits; return entry.result; }
    ++cache_misses; entry = {k, solve(t, day, horizon, 0, full, true)}; return entry.result;
}
}
