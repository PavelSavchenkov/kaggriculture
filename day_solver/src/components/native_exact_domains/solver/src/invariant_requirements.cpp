#include "invariant_requirements.hpp"

#include <algorithm>
#include <string>

namespace day_solver {
namespace {

void require_event(InvariantRequirements& result, OutcomeMetric metric,
                   int subject, int tile, int64_t quantity) {
    if (quantity <= 0) return;
    result.outcomes.push_back({
        {metric, static_cast<int16_t>(subject), static_cast<int16_t>(tile), -1},
        quantity, std::numeric_limits<int64_t>::max()});
}

void require_end_tile(InvariantRequirements& result, int tile,
                      ManagedTileKind kind, int item) {
    result.end_tiles.push_back({static_cast<int16_t>(tile), kind,
                                static_cast<int16_t>(item), std::nullopt});
}

void derive_melon(const ManagedTileState& state, int tile,
                  InvariantRequirements& result) {
    if (state.age_days < 0 || state.age_days > 10) {
        result.errors.push_back("managed tile " + std::to_string(tile) +
            " has a melon outside invariant ages 0-10");
        return;
    }
    if (state.age_days == 10) {
        // Watering may be needed first. Requiring the result, rather than a
        // WATER action, lets the solver choose the cheapest valid sequence.
        require_event(result, OutcomeMetric::HARVESTED, kag::MELON, tile, 6);
        return;
    }

    require_end_tile(result, tile, ManagedTileKind::CROP, kag::MELON);
    // Starting at age 6, one ordinary water on every remaining day is enough
    // to reach six at age 10. Earlier fertilizer bonuses can make today's
    // water unnecessary; this lower bound therefore does not script watering.
    const int64_t minimum_stored = std::max<int64_t>(1, state.age_days - 4);
    result.outcomes.push_back({
        {OutcomeMetric::END_TILE_STORED, kag::MELON,
         static_cast<int16_t>(tile), -1},
        minimum_stored, std::numeric_limits<int64_t>::max()});
}

void derive_strawberry(const ManagedTileState& state, int tile,
                       InvariantRequirements& result) {
    const int64_t age = state.age_days;
    if (age < 0) {
        result.errors.push_back("managed tile " + std::to_string(tile) +
            " has a negative strawberry age");
        return;
    }

    if (age <= 15)
        require_end_tile(result, tile, ManagedTileKind::CROP,
                         kag::STRAWBERRY);
    if ((age == 9 || age == 11 || age == 13 || age == 15) &&
        !state.watered_today)
        require_event(result, OutcomeMetric::CROP_WATERED,
                      kag::STRAWBERRY, tile, 1);
    if ((age == 10 || age == 12 || age == 14 || age == 16) &&
        state.stored_units > 0)
        require_event(result, OutcomeMetric::HARVESTED,
                      kag::STRAWBERRY, tile, state.stored_units);
    if (age >= 16) {
        if (age > 17)
            result.errors.push_back("managed tile " + std::to_string(tile) +
                " retained a strawberry past clear age 17");
        require_event(result, OutcomeMetric::DUG, NO_SUBJECT, tile, 1);
    }
}

void derive_carrot(const ManagedTileState& state, int tile,
                   InvariantRequirements& result) {
    const int64_t age = state.age_days;
    if (age < 2) {
        require_end_tile(result, tile, ManagedTileKind::CROP, kag::CARROT);
    } else if (age == 2) {
        result.alternatives.push_back({static_cast<int16_t>(tile), kag::CARROT,
                                       state.stored_units});
    } else {
        require_event(result, OutcomeMetric::HARVESTED, kag::CARROT, tile,
                      state.stored_units);
    }
}

void derive_animal(const ManagedTileState& state, int tile,
                   InvariantRequirements& result) {
    require_end_tile(result, tile, ManagedTileKind::PASTURE, state.animal);
    if (!state.fed_today)
        require_event(result, OutcomeMetric::FED, state.animal, tile, 1);
    if (!state.cared_today)
        require_event(result, OutcomeMetric::CARED, state.animal, tile, 1);
    if (state.fertilizer_available)
        require_event(result, OutcomeMetric::FERTILIZER_COLLECTED,
                      NO_SUBJECT, tile, 1);
    if (state.stored_units > 0) {
        const int product = kag::ANIMALS[state.animal - kag::GOOSE].product;
        require_event(result, OutcomeMetric::HARVESTED, product, tile,
                      state.stored_units);
    }
}

}  // namespace

InvariantRequirements derive_invariant_requirements(
    const PhysicalState& start) {
    InvariantRequirements result;
    for (size_t tile = 0; tile < start.managed_tiles.size(); ++tile) {
        const ManagedTileState& state = start.managed_tiles[tile].state;
        if (state.kind == ManagedTileKind::CROP) {
            if (state.crop == kag::MELON)
                derive_melon(state, tile, result);
            else if (state.crop == kag::STRAWBERRY)
                derive_strawberry(state, tile, result);
            else if (state.crop == kag::CARROT)
                derive_carrot(state, tile, result);
        } else if (state.kind == ManagedTileKind::PASTURE &&
                   state.animal != NO_SUBJECT) {
            derive_animal(state, tile, result);
        }
    }
    return result;
}

InvariantRequirements derive_invariant_requirements(const DayProblem& problem) {
    InvariantRequirements result = derive_invariant_requirements(problem.start);
    for (const EndTileRequirement& end : problem.required_end_tiles) {
        if (end.kind != ManagedTileKind::PASTURE ||
            (end.item != kag::COW && end.item != kag::SHEEP) ||
            end.tile < 0 ||
            static_cast<size_t>(end.tile) >= problem.start.managed_tiles.size())
            continue;
        const ManagedTileState& start =
            problem.start.managed_tiles[end.tile].state;
        if (start.kind == ManagedTileKind::PASTURE &&
            start.animal == end.item)
            continue;
        require_event(result, OutcomeMetric::FED, end.item, end.tile, 1);
        require_event(result, OutcomeMetric::CARED, end.item, end.tile, 1);
    }
    return result;
}

}  // namespace day_solver
