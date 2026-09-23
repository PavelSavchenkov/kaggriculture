#include "commitments.hpp"
#include <algorithm>
#include <cstdlib>

namespace kag::day_compiler {
void CommitmentAudit::begin(const Observation& dawn, const detail::BoundIntent& intent) {
    dawn_ = dawn; intent_ = intent; latest_ = own_farm(dawn);
    std::fill_n(harvest_, 100, 0); std::fill_n(fed_, 100, false);
    std::fill_n(cared_, 100, false); std::fill_n(established_, 100, false);
    std::fill_n(planted_, N_CROPS, 0);
}
void CommitmentAudit::record(const Observation& before, const Action& action, const Farm& after) {
    for (int product = 0; product < N_CROPS; ++product) planted_[product] += before.own.seeds[product] - after.seeds[product];
    for (int unit = 0; unit < std::min(action.n_units, before.self().n_units); ++unit) {
        if (action.units[unit].op != OP_HARVEST) continue;
        const int cell = before.self().pos_y[unit] * BOARD + before.self().pos_x[unit];
        const auto& original = dawn_.self().tiles[cell / BOARD][cell % BOARD];
        const auto& current = before.self().tiles[cell / BOARD][cell % BOARD];
        if (!same_entity(original, current) || (!current.has_animal && current.kind != T_PLANT)) continue;
        const int product = current.has_animal ? int(ANIMALS[current.what - GOOSE].product) : current.what;
        harvest_[cell] += std::max(0, int(after.inv[unit][product]) - int(before.own.inv[unit][product]));
    }
    for (int cell = 0; cell < 100; ++cell) {
        const auto& original = dawn_.self().tiles[cell / BOARD][cell % BOARD];
        const auto& current = after.tiles[cell / BOARD][cell % BOARD];
        if (original.has_animal && same_entity(original, current)) {
            fed_[cell] |= current.fed_today; cared_[cell] |= current.cared_today;
        }
        if (current.kind == T_PLANT && current.planted_day == dawn_.day)
            established_[cell] |= current.watered_today;
    }
    latest_ = after;
}
CommitmentReport CommitmentAudit::finish(const Observation& final) {
    CommitmentReport report;
    const int day = dawn_.day;
    int new_crops[N_CROPS]{}, animals[3]{};
    for (int cell = 0; cell < 100; ++cell) {
        const auto& original = dawn_.self().tiles[cell / BOARD][cell % BOARD];
        const auto& tile = final.self().tiles[cell / BOARD][cell % BOARD];
        const auto& last = latest_.tiles[cell / BOARD][cell % BOARD];
        if (tile.has_animal) ++animals[tile.what - GOOSE];
        if (tile.kind == T_PLANT && tile.planted_day == day) {
            ++new_crops[tile.what]; report.establishment_water += !established_[cell];
        }
        if (original.has_animal) {
            if (intent_.escape[cell]) report.escapes += same_entity(original, tile);
            else report.retained_animals += !same_entity(original, tile);
            if (intent_.serve[cell]) {
                report.feeds += !fed_[cell];
                if (!zero_value_care(original, day)) report.care += !cared_[cell];
            }
            const auto& animal = ANIMALS[original.what - GOOSE];
            const int since = day + 1 - original.planted_day - animal.first_yield_day;
            if (day < 29 && !intent_.escape[cell] && since >= 0 && since % animal.interval == 0) {
                const int expected = original.yield_units - harvest_[cell] + 1 + (fed_[cell] ? original.pending_care_bonus : 0);
                // Animal service promises FEED and useful CARE. Unlike FULL
                // crop production, it does not promise lossless harvesting of
                // all existing stock before every production (design 11.2).
                report.animal_production += tile.yield_units < std::min(expected,animal.max_held);
            }
            continue;
        }
        if (original.kind != T_PLANT || intent_.crops[cell].mode == CropMode::Retire) continue;
        const auto goal = intent_.crops[cell]; const auto& crop = CROPS[original.what];
        if (!crop.ongoing) {
            if (day - original.planted_day >= goal.harvest_age) {
                if (harvest_[cell] >= goal.min_yield) continue;
                // The age is a growth target, not a forced harvest date. A
                // retained promise must still be collectible at the next dawn.
                bool preserved=day<29 && same_entity(original,tile) && tile.yield_units>=goal.min_yield;
                if(preserved && tile.max_lifespan_step>=0) {
                    const int expiry=tile.max_lifespan_step-final.step;
                    const int first_loss=expiry<0?(-expiry&1):expiry;
                    const int deadline=first_loss+2*(tile.yield_units-goal.min_yield);
                    const int farmer=final.self().pos_y[0]*BOARD+final.self().pos_x[0];
                    preserved=std::min(distance(farmer,cell),1+shed_distance(cell))<=deadline;
                }
                report.crop_yield += !preserved;
            }
            else if (!same_entity(original, tile)) ++report.retained_crops;
            else if (!biology_.one_shot(tile, day + 1, goal.harvest_age, goal.min_yield).count) ++report.crop_yield;
            continue;
        }
        const int last_production = original.planted_day + crop.first_yield_day + crop.interval * (crop.max_yield - 1);
        if (day >= std::min(29, last_production)) continue;
        if (!same_entity(original, tile)) { ++report.retained_crops; continue; }
        const int since = day + 1 - original.planted_day - crop.first_yield_day;
        const bool productive = since >= 0 && since % crop.interval == 0 && day + 1 <= last_production;
        if (productive) {
            const int incoming = goal.mode == CropMode::Full ? 2 : 1;
            const int expected = original.yield_units - harvest_[cell] + incoming;
            report.crop_production += expected > crop.max_yield || tile.yield_units < expected;
            if (goal.mode == CropMode::Full)
                report.crop_production += !last.watered_today || last.fertilized_until_day < day;
        }
        if (!biology_.ongoing(tile, day + 1, goal.mode == CropMode::Full).count) ++report.crop_yield;
    }
    for (int product = 0; product < N_CROPS; ++product) {
        report.new_crop_counts += std::abs(new_crops[product] - intent_.new_crops[product]);
        report.new_crop_counts += std::abs(planted_[product] - intent_.new_crops[product]);
    }
    for (int species = 0; species < 3; ++species)
        report.animal_targets += std::abs(animals[species] - intent_.animal_target[species]);
    report.land = final.self().n_quadrants != dawn_.self().n_quadrants + int(intent_.buy_land);
    return report;
}
}
