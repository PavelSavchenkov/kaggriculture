#include <array>
#include <cassert>
#include <exception>
#include <iostream>
#include <limits>

#include "problem_json.hpp"
#include "problem_validation.hpp"
#include "replay.hpp"

using namespace day_solver;

namespace {

constexpr int64_t LIMIT = MAX_INPUT_COUNT;
using Schedule = std::array<kag::Action, HOURS>;

ManagedTileState animal(int item) {
    ManagedTileState state;
    state.kind = structure_for(item);
    state.animal = item;
    return state;
}

ManagedTileState crop(int item) {
    ManagedTileState state;
    state.kind = ManagedTileKind::CROP;
    state.crop = item;
    state.stored_units = 1;
    return state;
}

DayProblem problem(const ManagedTileState& start, const ManagedTileState& end) {
    DayProblem result;
    result.start.shed_capacity = std::numeric_limits<int16_t>::max();
    result.start.managed_tiles.push_back({4, 4, start});
    const int item = end.kind == ManagedTileKind::CROP ? end.crop : end.animal;
    result.required_end_tiles.push_back({0, end.kind, static_cast<int16_t>(item), end});
    return result;
}

ReplayResult replay(const DayProblem& input, const Schedule& schedule = {}) {
    const auto parsed = parse_problem_json(serialize_problem_json(input));
    assert(validate_problem(parsed).empty());
    assert(parsed.start.managed_tiles == input.start.managed_tiles);
    return replay_schedule(parsed, schedule);
}

void accepted(const ReplayResult& result) {
    assert(result.candidate.replay.strict_valid);
    assert(result.requirements_satisfied && result.invariants_satisfied);
}

}  // namespace

int main() {
    for (int64_t age : std::array<int64_t, 3>{32767, 32768, LIMIT - 1}) {
        auto start = animal(kag::GOOSE);
        start.age_days = age;
        auto end = start;
        end.age_days = age + 1;
        end.consecutive_dry_days = 1;
        end.stored_units = 1;  // A mature goose produces every night.
        end.fertilizer_available = true;
        accepted(replay(problem(start, end)));
    }
    {
        auto start = animal(kag::GOOSE);
        start.age_days = LIMIT;
        const auto result = replay(problem(start, start));
        assert(result.candidate.replay.strict_valid && !result.requirements_satisfied);
        assert(result.candidate.end.physical.managed_tiles[0].state.age_days == LIMIT + 1);
    }

    for (int64_t dry : std::array<int64_t, 3>{32767, 32768, LIMIT}) {
        for (int item = 0; item < kag::N_CROPS; ++item) {
            auto start = crop(item);
            start.consecutive_dry_days = dry;
            ManagedTileState end;
            end.kind = ManagedTileKind::WEED;
            accepted(replay(problem(start, end)));
        }
        for (int item = kag::GOOSE; item <= kag::SHEEP; ++item) {
            auto start = animal(item);
            start.consecutive_dry_days = dry;
            ManagedTileState end;
            end.kind = start.kind;
            accepted(replay(problem(start, end)));
        }
    }

    for (int item = kag::GOOSE; item <= kag::SHEEP; ++item) {
        auto start = animal(item);
        start.age_days = item == kag::GOOSE ? 3 : item == kag::COW ? 7 : 5;
        start.pending_care_bonus = LIMIT;
        start.fed_today = true;
        auto end = start;
        ++end.age_days;
        end.stored_units = item == kag::GOOSE ? 4 : 6;
        end.pending_care_bonus = 0;
        end.fed_today = false;
        end.fertilizer_available = true;
        // Base production plus LIMIT must be capped before narrowing the yield.
        accepted(replay(problem(start, end)));

        start = animal(item);
        start.pending_care_bonus = LIMIT - 1;
        end = start;
        end.age_days = 1;
        end.pending_care_bonus = LIMIT;
        end.fertilizer_available = true;
        auto input = problem(start, end);
        input.start.shed[kag::WHEAT] = 1;
        input.tile_work = {{0, {{kag::OP_CARE, NO_SUBJECT, 1, NO_SUBJECT, 0},
                                {kag::OP_FEED, NO_SUBJECT, 1, NO_SUBJECT, 0}}}};
        Schedule schedule{};
        schedule[0].units[0] = {kag::OP_PICKUP, kag::WHEAT, 1};
        schedule[1].units[0] = {kag::OP_CARE, 0, 1};
        schedule[2].units[0] = {kag::OP_FEED, 0, 1};
        accepted(replay(input, schedule));

        input.start.managed_tiles[0].state.pending_care_bonus = LIMIT;
        const auto result = replay(input, schedule);
        assert(result.candidate.replay.strict_valid && !result.requirements_satisfied);
        assert(result.candidate.end.physical.managed_tiles[0].state.pending_care_bonus == LIMIT + 1);
    }

    for (int item = 0; item < kag::N_CROPS; ++item) {
        auto start = crop(item);
        start.watered_today = true;
        start.fertilizer_days_remaining = LIMIT;
        auto end = start;
        end.age_days = 1;
        end.watered_today = false;
        end.fertilizer_days_remaining = LIMIT - 1;
        auto input = problem(start, end);
        input.start.shed[kag::FERTILIZER] = 1;
        input.tile_work = {{0, {{kag::OP_FERTILIZE, NO_SUBJECT, 1, NO_SUBJECT, 0}}}};
        Schedule schedule{};
        schedule[0].units[0] = {kag::OP_PICKUP, kag::FERTILIZER, 1};
        schedule[1].units[0] = {kag::OP_FERTILIZE, 0, 1};
        accepted(replay(input, schedule));
    }

    using Field = int64_t ManagedTileState::*;
    for (Field field : std::array<Field, 4>{&ManagedTileState::age_days,
            &ManagedTileState::consecutive_dry_days, &ManagedTileState::pending_care_bonus,
            &ManagedTileState::fertilizer_days_remaining}) {
        auto state = field == &ManagedTileState::fertilizer_days_remaining
            ? crop(kag::WHEAT) : animal(kag::COW);
        state.*field = LIMIT;
        auto input = problem(state, state);
        assert(validate_problem(input).empty());
        input.start.managed_tiles[0].state.*field = LIMIT + 1;
        assert(!validate_problem(input).empty());
        bool rejected = false;
        try {
            (void)parse_problem_json(serialize_problem_json(input));
        } catch (const std::exception&) {
            rejected = true;
        }
        assert(rejected);
    }
    std::cout << "Tile counter boundary checks passed\n";
}
