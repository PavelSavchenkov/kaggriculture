#include "problem_validation.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <string>

namespace day_solver {
namespace {

using Issues = std::vector<ValidationIssue>;

void add(Issues& issues, std::string path, std::string message) {
    issues.push_back({std::move(path), std::move(message)});
}

bool supported_crop(int subject) {
    return subject >= 0 && subject < kag::N_CROPS;
}

bool supported_animal(int subject) {
    return subject >= kag::GOOSE && subject <= kag::SHEEP;
}

bool supported_product(int subject) {
    return subject >= 0 && subject <= kag::FERTILIZER;
}

bool supported_item(int subject) {
    return supported_product(subject) || supported_animal(subject);
}

bool tile_filter_allowed(OutcomeMetric metric) {
    switch (metric) {
        case OutcomeMetric::PLANTED:
        case OutcomeMetric::HARVESTED:
        case OutcomeMetric::ANIMAL_PLACED:
        case OutcomeMetric::FED:
        case OutcomeMetric::CARED:
        case OutcomeMetric::FERTILIZER_COLLECTED:
        case OutcomeMetric::CROP_WATERED:
        case OutcomeMetric::CROP_FERTILIZED:
        case OutcomeMetric::PASTURE_BUILT:
        case OutcomeMetric::COOP_BUILT:
        case OutcomeMetric::DUG:
        case OutcomeMetric::END_TILE_STORED:
            return true;
        default:
            return false;
    }
}

bool subject_allowed(const OutcomeKey& key) {
    switch (key.metric) {
        case OutcomeMetric::PLANTED:
        case OutcomeMetric::CROP_WATERED:
        case OutcomeMetric::CROP_FERTILIZED:
            return key.subject == NO_SUBJECT || supported_crop(key.subject);
        case OutcomeMetric::END_SEEDS:
            return key.subject == NO_SUBJECT ||
                   (key.subject >= 0 && key.subject < kag::N_CROPS);
        case OutcomeMetric::HARVESTED:
        case OutcomeMetric::PRODUCT_DEPOSITED:
        case OutcomeMetric::PICKUPS:
        case OutcomeMetric::DROPS:
            return key.subject == NO_SUBJECT || supported_product(key.subject);
        case OutcomeMetric::END_SHED:
            return key.subject == NO_SUBJECT ||
                   (key.subject >= 0 && key.subject < kag::N_ITEMS);
        case OutcomeMetric::END_TILE_STORED:
            return key.subject == NO_SUBJECT || supported_product(key.subject);
        case OutcomeMetric::ANIMAL_PLACED:
        case OutcomeMetric::FED:
        case OutcomeMetric::CARED:
            return key.subject == NO_SUBJECT || supported_animal(key.subject);
        case OutcomeMetric::PURCHASED:
            return key.subject == NO_SUBJECT || supported_item(key.subject);
        case OutcomeMetric::FERTILIZER_COLLECTED:
        case OutcomeMetric::PASTURE_BUILT:
        case OutcomeMetric::COOP_BUILT:
        case OutcomeMetric::DUG:
        case OutcomeMetric::END_CASH:
        case OutcomeMetric::HIRES:
        case OutcomeMetric::UNIT_ACTIONS:
        case OutcomeMetric::TRAVEL:
        case OutcomeMetric::SPARE_TURNS:
        case OutcomeMetric::MIN_SHED_HEADROOM:
            return key.subject == NO_SUBJECT;
    }
    return false;
}

bool end_only(OutcomeMetric metric) {
    return metric == OutcomeMetric::END_SHED ||
           metric == OutcomeMetric::END_SEEDS ||
           metric == OutcomeMetric::END_CASH ||
           metric == OutcomeMetric::END_TILE_STORED;
}

std::string key_id(const OutcomeKey& key) {
    return std::to_string(static_cast<int>(key.metric)) + ":" +
           std::to_string(key.subject) + ":" +
           std::to_string(key.tile) + ":" +
           std::to_string(key.through_hour);
}

void validate_key(const OutcomeKey& key, size_t tile_count,
                  const std::string& path, Issues& issues) {
    if (!subject_allowed(key))
        add(issues, path + ".subject", "invalid subject for metric");
    if (key.tile != NO_TILE &&
        (key.tile < 0 || static_cast<size_t>(key.tile) >= tile_count))
        add(issues, path + ".tile", "must be -1 or a managed_tiles index");
    if (key.tile != NO_TILE && !tile_filter_allowed(key.metric))
        add(issues, path + ".tile", "metric does not support a tile filter");
    if (key.metric == OutcomeMetric::END_TILE_STORED && key.tile == NO_TILE)
        add(issues, path + ".tile", "end_tile_stored requires a tile");
    if (key.through_hour < -1 || key.through_hour >= HOURS)
        add(issues, path + ".through_hour", "must be -1 or in [0, 23]");
    if (end_only(key.metric) && key.through_hour != -1)
        add(issues, path + ".through_hour", "end-state metric requires -1");
}

void validate_tile(const ManagedTile& tile, const std::string& path,
                   Issues& issues) {
    const ManagedTileState& state = tile.state;
    if (tile.x < 0 || tile.x >= kag::BOARD ||
        tile.y < 0 || tile.y >= kag::BOARD)
        add(issues, path, "coordinates are outside the board");
    const std::array<int64_t, 5> counters{state.age_days, state.stored_units,
        state.consecutive_dry_days, state.pending_care_bonus, state.fertilizer_days_remaining};
    if (std::any_of(counters.begin(), counters.end(), [](int64_t value) {
            return value < 0 || value > MAX_INPUT_COUNT;
        }))
        add(issues, path + ".state", "relative counters must be in [0, 2147483647]");

    if (state.kind == ManagedTileKind::CROP) {
        if (!supported_crop(state.crop))
            add(issues, path + ".state.crop", "unknown crop");
        if (state.animal != NO_SUBJECT)
            add(issues, path + ".state.animal", "crop tile cannot contain an animal");
        if (state.fed_today || state.cared_today ||
            state.fertilizer_available || state.pending_care_bonus != 0)
            add(issues, path + ".state", "crop has animal-only state");
        if (supported_crop(state.crop) &&
            state.stored_units > kag::CROPS[state.crop].max_yield)
            add(issues, path + ".state.stored_units", "exceeds crop maximum yield");
        return;
    }

    if (animal_structure(state.kind)) {
        if (state.crop != NO_SUBJECT)
            add(issues, path + ".state.crop", "pasture cannot contain a crop");
        if (state.watered_today || state.fertilizer_days_remaining != 0)
            add(issues, path + ".state", "pasture has crop-only state");
        if (state.animal == NO_SUBJECT) {
            if (state.age_days != 0 || state.stored_units != 0 ||
                state.consecutive_dry_days != 0 ||
                state.pending_care_bonus != 0 || state.fed_today ||
                state.cared_today || state.fertilizer_available)
                add(issues, path + ".state", "empty pasture has animal state");
        } else {
            if (!supported_animal(state.animal) || state.kind != structure_for(state.animal))
                add(issues, path + ".state.animal", "animal does not match structure");
            if (supported_animal(state.animal)) {
                const int animal_index = state.animal - kag::GOOSE;
                if (state.stored_units > kag::ANIMALS[animal_index].max_held)
                    add(issues, path + ".state.stored_units", "exceeds animal product capacity");
            }
        }
        return;
    }

    if (state.kind != ManagedTileKind::EMPTY &&
        state.kind != ManagedTileKind::WEED &&
        state.kind != ManagedTileKind::LOCKED) {
        add(issues, path + ".state.kind", "unknown managed tile kind");
        return;
    }
    if (state.crop != NO_SUBJECT || state.animal != NO_SUBJECT ||
        state.age_days != 0 || state.stored_units != 0 ||
        state.consecutive_dry_days != 0 || state.pending_care_bonus != 0 ||
        state.fertilizer_days_remaining != 0 || state.watered_today ||
        state.fed_today || state.cared_today || state.fertilizer_available)
        add(issues, path + ".state", "empty, weed, or locked tile has crop/animal state");
}

bool valid_market_item(const MarketEvent& event) {
    switch (event.market_op) {
        case kag::M_HIRE:
            return event.item == NO_SUBJECT && event.quantity == 1;
        case kag::M_BUY_LAND:
            return event.item >= 1 && event.item <= 3 && event.quantity == 1;
        case kag::M_BUY_SEED:
            return supported_crop(event.item);
        case kag::M_BUY_PRODUCT:
            return event.item == kag::WHEAT || event.item == kag::FERTILIZER;
        case kag::M_BUY_ANIMAL:
            return supported_animal(event.item);
        default:
            return false;
    }
}

void validate_land(const DayProblem& problem, Issues& issues) {
    std::vector<size_t> purchases;
    for (size_t index = 0; index < problem.market_plan.size(); ++index)
        if (problem.market_plan[index].market_op == kag::M_BUY_LAND)
            purchases.push_back(index);
    std::sort(purchases.begin(), purchases.end(), [&](size_t a, size_t b) {
        const auto& first = problem.market_plan[a];
        const auto& second = problem.market_plan[b];
        return std::pair{first.hour, first.order_index} <
               std::pair{second.hour, second.order_index};
    });
    for (size_t index = 1; index < purchases.size(); ++index)
        if (problem.market_plan[purchases[index]].item !=
            problem.market_plan[purchases[index - 1]].item + 1)
            add(issues, "buy_schedule[" + std::to_string(purchases[index]) + "].item",
                "land purchases must unlock consecutive quadrants in market order");
    bool compatible = false;
    for (int initial = 1; initial <= 4; ++initial) {
        if (!purchases.empty() && problem.market_plan[purchases[0]].item != initial)
            continue;
        bool matches = true;
        for (const auto& tile : problem.start.managed_tiles) {
            if (tile.x < 0 || tile.x >= kag::BOARD || tile.y < 0 || tile.y >= kag::BOARD)
                continue;
            const bool owned = kag::quadrant_of(tile.x, tile.y, kag::BOARD) < initial;
            if (owned == (tile.state.kind == ManagedTileKind::LOCKED)) matches = false;
        }
        compatible = compatible || matches;
    }
    if (!compatible)
        add(issues, "start.managed_tiles", "tile ownership disagrees with the fixed land purchases or quadrant order");
}

bool valid_tile_work_action(const TileWorkAction& action) {
    if (action.quantity != 1) return false;
    switch (action.op) {
        case kag::OP_PLANT:
            return supported_crop(action.arg) &&
                   action.output_item == NO_SUBJECT &&
                   action.output_quantity == 0;
        case kag::OP_PLACE:
            return supported_animal(action.arg) &&
                   action.output_item == NO_SUBJECT &&
                   action.output_quantity == 0;
        case kag::OP_HARVEST:
            return supported_product(action.arg) &&
                   action.output_item == action.arg &&
                   action.output_quantity > 0;
        case kag::OP_COLLECT_FERTILIZER:
            return action.arg == NO_SUBJECT &&
                   action.output_item == kag::FERTILIZER &&
                   action.output_quantity == 1;
        case kag::OP_WATER:
        case kag::OP_FERTILIZE:
        case kag::OP_DIG:
        case kag::OP_BUILD_PASTURE:
        case kag::OP_BUILD_COOP:
        case kag::OP_FEED:
        case kag::OP_CARE:
            return action.arg == NO_SUBJECT &&
                   action.output_item == NO_SUBJECT &&
                   action.output_quantity == 0;
        default:
            return false;
    }
}

}  // namespace

std::vector<ValidationIssue> validate_problem(const DayProblem& problem) {
    Issues issues;
    if (problem.format_version != FORMAT_VERSION)
        add(issues, "format_version", "unsupported format version");
    if (problem.worker_count < 1 || problem.worker_count > kag::MAX_UNITS)
        add(issues, "worker_count", "must be in [1, 40]");

    const PhysicalState& start = problem.start;
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        if (start.shed[item] < 0 || start.shed[item] > MAX_INPUT_COUNT)
            add(issues, "start.shed[" + std::to_string(item) + "]", "must be in [0, 2147483647]");
    }

    for (int crop = 0; crop < kag::N_CROPS; ++crop) {
        if (start.seeds[crop] < 0 || start.seeds[crop] > MAX_INPUT_COUNT)
            add(issues, "start.seeds[" + std::to_string(crop) + "]", "must be in [0, 2147483647]");
    }

    std::set<int> coordinates;
    for (size_t index = 0; index < start.managed_tiles.size(); ++index) {
        const ManagedTile& tile = start.managed_tiles[index];
        const std::string path = "start.managed_tiles[" + std::to_string(index) + "]";
        validate_tile(tile, path, issues);
        if (tile.x >= 0 && tile.x < kag::BOARD &&
            tile.y >= 0 && tile.y < kag::BOARD &&
            !coordinates.insert(tile.y * kag::BOARD + tile.x).second)
            add(issues, path, "duplicates another managed coordinate");
    }

    std::set<std::string> outcome_keys;
    for (size_t index = 0; index < problem.required_outcomes.size(); ++index) {
        const OutcomeBound& bound = problem.required_outcomes[index];
        const std::string path = "required_outcomes[" + std::to_string(index) + "]";
        validate_key(bound.key, start.managed_tiles.size(), path + ".key", issues);
        if (bound.lower > bound.upper)
            add(issues, path, "lower exceeds upper");
        if (!outcome_keys.insert(key_id(bound.key)).second)
            add(issues, path + ".key", "duplicate outcome key");
    }

    std::set<int> end_tiles;
    for (size_t index = 0; index < problem.required_end_tiles.size(); ++index) {
        const EndTileRequirement& required = problem.required_end_tiles[index];
        const std::string path = "end_tiles[" + std::to_string(index) + "]";
        if (required.tile < 0 ||
            static_cast<size_t>(required.tile) >= start.managed_tiles.size()) {
            add(issues, path + ".tile", "must name a managed_tiles index");
        } else if (!end_tiles.insert(required.tile).second) {
            add(issues, path + ".tile", "duplicate end-tile requirement");
        }
        if (!required.exact_state) {
            add(issues, path + ".state", "exact successor state is required");
        } else if (required.tile >= 0 &&
                   static_cast<size_t>(required.tile) <
                       start.managed_tiles.size()) {
            ManagedTile tile = start.managed_tiles[required.tile];
            tile.state = *required.exact_state;
            validate_tile(tile, path, issues);
            if (required.kind != required.exact_state->kind)
                add(issues, path + ".state.kind", "derived kind mismatch");
            const int item = required.kind == ManagedTileKind::CROP
                ? required.exact_state->crop
                : animal_structure(required.kind)
                    ? required.exact_state->animal : NO_SUBJECT;
            if (required.item != item)
                add(issues, path + ".state", "derived item mismatch");
        }
        switch (required.kind) {
            case ManagedTileKind::CROP:
                if (!supported_crop(required.item))
                    add(issues, path + ".item", "crop end state needs a supported crop");
                break;
            case ManagedTileKind::PASTURE:
            case ManagedTileKind::COOP:
                if (required.item != NO_SUBJECT && !supported_animal(required.item))
                    add(issues, path + ".item", "structure item must be -1 or an animal");
                break;
            case ManagedTileKind::EMPTY:
            case ManagedTileKind::WEED:
            case ManagedTileKind::LOCKED:
                if (required.item != NO_SUBJECT)
                    add(issues, path + ".item", "empty, weed, or locked end state requires -1");
                break;
            default:
                add(issues, path + ".kind", "unknown managed tile kind");
        }
    }
    if (end_tiles.size() != start.managed_tiles.size())
        add(issues, "end_tiles", "must specify every managed tile exactly once");

    std::set<int> work_tiles;
    for (size_t index = 0; index < problem.tile_work.size(); ++index) {
        const TileWork& work = problem.tile_work[index];
        const std::string path = "tile_work[" + std::to_string(index) + "]";
        if (work.tile < 0 ||
            static_cast<size_t>(work.tile) >= start.managed_tiles.size()) {
            add(issues, path + ".tile", "must name a managed_tiles index");
        } else if (!work_tiles.insert(work.tile).second) {
            add(issues, path + ".tile", "duplicate tile-work entry");
        }
        if (work.actions.empty())
            add(issues, path + ".actions", "must not be empty; omit idle tiles");
        for (size_t action = 0; action < work.actions.size(); ++action)
            if (!valid_tile_work_action(work.actions[action]))
                add(issues, path + ".actions[" + std::to_string(action) + "]",
                    "invalid operation, argument, quantity, or output");
    }

    for (int hour = 0; hour < HOURS; ++hour) {
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            const InventoryCount value = problem.shed_availability[hour][item];
            const std::string path = "shed_availability[" +
                std::to_string(hour) + "][" + std::to_string(item) + "]";
            if (value < 0 || value > MAX_INPUT_COUNT)
                add(issues, path, "must be in [0, 2147483647]");
            if (hour && value < problem.shed_availability[hour - 1][item])
                add(issues, path, "must be cumulative and nondecreasing");
        }
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (problem.end_shed[item] < 0 || problem.end_shed[item] > MAX_INPUT_COUNT)
            add(issues, "end_shed[" + std::to_string(item) + "]",
                "must be in [0, 2147483647]");
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        if (problem.end_seeds[crop] < 0 || problem.end_seeds[crop] > MAX_INPUT_COUNT)
            add(issues, "end_seeds[" + std::to_string(crop) + "]",
                "must be in [0, 2147483647]");

    std::set<std::pair<int, int>> market_slots;
    int hires = 0;
    for (size_t index = 0; index < problem.market_plan.size(); ++index) {
        const MarketEvent& event = problem.market_plan[index];
        const std::string path = "buy_schedule[" + std::to_string(index) + "]";
        if (event.hour < 0 || event.hour >= HOURS)
            add(issues, path + ".hour", "must be in [0, 23]");
        if (event.order_index < 0 || event.order_index >= 10)
            add(issues, path + ".order_index", "must be in [0, 9]");
        if (!market_slots.insert({event.hour, event.order_index}).second)
            add(issues, path, "duplicates an hour and order_index");
        if (event.quantity <= 0)
            add(issues, path + ".quantity", "must be positive");
        if (!valid_market_item(event))
            add(issues, path, "invalid market operation, item, or quantity");
        if (event.cash_delta != 0)
            add(issues, path, "cash is external in format v3");
        if (event.market_op == kag::M_HIRE) ++hires;
    }
    if (hires != problem.worker_count - 1)
        add(issues, "buy_schedule", "must contain worker_count - 1 hires");
    validate_land(problem, issues);
    if (!problem.sale_targets.empty())
        add(issues, "sale_targets", "not part of format v3");

    return issues;
}

}  // namespace day_solver
