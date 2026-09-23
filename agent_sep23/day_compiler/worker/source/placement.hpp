#pragma once
#include "agent.hpp"
#include <algorithm>
#include <cassert>
#include <climits>

namespace kag::agents::sep22_worker {
inline constexpr auto ANIMAL_ZONE_RANK = [] {
    std::array<uint8_t, 100> rank{};
    auto cost = [](int c) {
        const int corner = GEOMETRY.corner[c];
        const int dx = c % 10 > corner % 10 ? c % 10 - corner % 10 : corner % 10 - c % 10;
        const int dy = c / 10 > corner / 10 ? c / 10 - corner / 10 : corner / 10 - c / 10;
        return 100 * std::max(dx, dy) + 10 * (dx + dy);
    };
    for (int c = 0; c < 100; ++c) for (int other = 0; other < 100; ++other)
        if (GEOMETRY.corner[c] == GEOMETRY.corner[other] &&
            (cost(other) < cost(c) || (cost(other) == cost(c) && other < c))) ++rank[c];
    return rank;
}();

// Select sites jointly with clearance jobs. Animals prefer short shed trips;
// crops prefer nearby cleared crop sites and spread across owned quadrants.
inline void place_day(const agent::AgentObservation& o, DayPlan& plan, int reserve_per_quadrant = 0,
                      bool release_aware = true, PlacementStyle style = PlacementStyle::Staged) {
    if (!plan.relocate_new) return;
    int order[100], items[MAX_JOBS], count_new = 0;
    bool has_melon = false;
    for (int j = 0; j < plan.count; ++j) {
        const auto& job = plan.jobs[j];
        if (!job.new_site) continue;
        int item = -1;
        for (int k = 0; k < job.count; ++k)
            if (job.steps[k].op == OP_PLANT || (job.steps[k].op == OP_PLACE && is_animal(job.steps[k].arg))) item = job.steps[k].arg;
        if (item < 0) continue;
        assert(count_new < 100);
        order[count_new++] = j; items[j] = item; has_melon |= item == MELON;
    }
    if (!count_new) { plan.relocate_new = false; return; }
    const bool staged = style == PlacementStyle::Staged;
    Tile tiles[100]; std::copy_n(&o.self().tiles[0][0], 100, tiles);
    bool opening=o.self().n_quadrants==1;
    for(const auto& t:tiles)opening &= t.kind==T_EMPTY || t.kind==T_LOCKED;
    bool reserved[100]{};
    int clearing[100]; std::fill_n(clearing, 100, -1);
    const int quadrants = std::min(4, o.self().n_quadrants + plan.buy_land);
    for (int c = 0; c < 100; ++c)
        if (tiles[c].kind == T_LOCKED && quadrant_of(c%10,c/10,10) < quadrants) tiles[c] = {};
    for (int j = 0; j < plan.count; ++j) {
        const auto& job = plan.jobs[j];
        if (job.new_site || job.depot || job.tile < 0 || !job.count) continue;
        const int c = job.tile, op = job.steps[job.count-1].op;
        if (op == OP_DIG || (op == OP_HARVEST && tiles[c].kind == T_PLANT && !CROPS[tiles[c].what].ongoing)) {
            clearing[c] = j; tiles[c] = {};
        }
    }
    const int corners[] = {44,45,54,55};
    if (staged && opening) {
        auto priority = [&](int j) { return is_animal(items[j]) ? 0 : items[j] == MELON ? 1 : 2; };
        std::sort(order,order+count_new,[&](int a,int b) { return priority(a)!=priority(b) ? priority(a)<priority(b) : a<b; });
    }
    for (int animal = 1; animal >= 0; --animal) {
        int index = 0;
        for (int n = 0; n < count_new; ++n) {
            const int j = order[n];
            auto& job = plan.jobs[j];
            const int item = items[j];
            if (item < 0 || is_animal(item) != bool(animal)) continue;
            const auto want = animal ? (ANIMALS[item-GOOSE].structure == ST_COOP ? T_COOP : T_PASTURE) : T_EMPTY;
            const int anchor = corners[index++ % quadrants];
            int best = -1, score = INT_MAX;
            for (int c = 0; c < 100; ++c) {
                const auto& t = tiles[c];
                if (reserved[c] || t.has_animal || t.kind == T_LOCKED || t.kind == T_PLANT) continue;
                const bool housing = t.kind == T_COOP || t.kind == T_PASTURE;
                const bool reuse = animal && t.kind == want;
                int cost = 100 * (animal ? shed_distance(c) : distance(anchor,c));
                // Soft reserve: requested establishments can still fill every
                // site, and reuse of a cleared crop remains the stronger choice.
                if (staged && !animal && ANIMAL_ZONE_RANK[c] < 6) cost += 2000;
                if (staged && opening && has_melon && item == WHEAT) cost -= 200*shed_distance(c);
                // Empty opening: arrange all animals jointly in short arms
                // along the shed's row/column, leaving the diagonal for crops.
                if(opening && animal && c%10!=GEOMETRY.corner[c]%10 && c/10!=GEOMETRY.corner[c]/10)cost+=10;
                cost += t.kind == T_WEED || (housing && !reuse) ? 100 : 0;
                cost += animal && !reuse ? 100 : 0;
                if (clearing[c] >= 0) cost += animal ? 100 : -10000;
                if (!animal && ANIMAL_ZONE_RANK[c] < reserve_per_quadrant) cost += 1000;
                if (!animal) cost -= shed_distance(c);
                if(release_aware && plan.land_hour>=0 && o.self().tiles[c/10][c%10].kind==T_LOCKED) {
                    // Waiting for land should not make a distant site look
                    // cheaper. Keep the distance term and add a release cost.
                    // Permanent animals may wait for a better shed-access
                    // site. Crops retain the immediate-availability preference.
                    if (!(staged && animal)) cost+=100*plan.land_hour;
                    if (staged && !animal && plan.land_hour>=16) cost+=10000;
                    const int setup=(t.kind==T_WEED || (housing && !reuse))+(animal && !reuse);
                    if(plan.land_hour+1+job.count+setup>24)cost+=100000;
                }
                if (cost < score) { best = c; score = cost; }
            }
            if (best < 0) { job.tile = -1; continue; }
            job.tile = best; job.predecessor = clearing[best]; reserved[best] = true;
            UnitAction steps[MAX_STEPS]; int count = 0;
            const auto& t = tiles[best];
            if (t.kind == T_WEED || ((t.kind == T_COOP || t.kind == T_PASTURE) && (!animal || t.kind != want)))
                steps[count++] = {OP_DIG,0,1};
            if (animal && t.kind != want)
                steps[count++] = {uint8_t(want == T_COOP ? OP_BUILD_COOP : OP_BUILD_PASTURE),0,1};
            for (int k = 0; k < job.count; ++k)
                if (job.steps[k].op != OP_BUILD_COOP && job.steps[k].op != OP_BUILD_PASTURE) steps[count++] = job.steps[k];
            assert(count <= MAX_STEPS); job.count = count;
            std::copy_n(steps, count, job.steps);

        }
    }
    plan.relocate_new = false;
}
}
