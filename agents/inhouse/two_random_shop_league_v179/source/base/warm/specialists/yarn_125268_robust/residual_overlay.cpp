#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/yarn_125268_robust/residual_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::two_random_shop_league_v179::base::warm::yarn_125268_robust::detail::residual {
namespace tape {
#include "agents/inhouse/two_random_shop_league_v179/source/base/warm/specialists/yarn_125268_robust/tape.inc"
}
namespace {

struct Day17Target {
    int8_t x = 0;
    int8_t y = 0;
};

constexpr std::array<Day17Target, 15> DAY17_TARGETS = {{
    {3, 5}, {4, 5}, {2, 5}, {4, 7}, {0, 9},
    {1, 5}, {4, 8}, {0, 5}, {4, 9}, {0, 6},
    {3, 9}, {0, 0}, {0, 7}, {3, 0}, {3, 8},
}};

int nominal_hires(int step) {
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    cursor += 3 * units;
    int result = 0;
    for (int order = 0; order < orders; ++order) {
        result += tape::TAPE_DATA[cursor] == kag::M_HIRE;
        cursor += 3;
    }
    return result;
}

}  // namespace

void Overlay::reset(uint8_t player) {
    player_ = player;
    stats_ = {};
}

void Overlay::modify(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    if (observation.player != player_) std::abort();
    if (!(parameters_.suppress_cleanup_hire_day_mask &
        (uint32_t{1} << observation.day)))
        return;
    int action_hires = 0;
    for (int order = 0; order < action.n_orders; ++order)
        action_hires += action.orders[order].op == kag::M_HIRE;
    int excess = action_hires - nominal_hires(observation.step);
    if (excess > 0 && observation.day == 17) {
        if (!parameters_.day17_target_mask) return;
        int weed_count = 0;
        int target_index = -1;
        for (int index = 0; index < static_cast<int>(DAY17_TARGETS.size()); ++index) {
            const Day17Target& target = DAY17_TARGETS[index];
            if (observation.self().tiles[target.y][target.x].kind != kag::T_WEED)
                continue;
            ++weed_count;
            target_index = index;
        }
        if (parameters_.require_single_day17_target && weed_count != 1) {
            ++stats_.rejected_multiple_targets;
            return;
        }
        if (target_index < 0 ||
            !(parameters_.day17_target_mask & (uint16_t{1} << target_index))) {
            ++stats_.rejected_target_mask;
            return;
        }
    }
    while (excess-- > 0) {
        int remove = action.n_orders - 1;
        while (remove >= 0 && action.orders[remove].op != kag::M_HIRE) --remove;
        if (remove < 0) std::abort();
        for (int order = remove + 1; order < action.n_orders; ++order)
            action.orders[order - 1] = action.orders[order];
        --action.n_orders;
        ++stats_.suppressed_cleanup_hires;
        for (int y = 0; y < kag::BOARD; ++y)
            for (int x = 0; x < kag::BOARD; ++x) {
                if (observation.self().tiles[y][x].kind != kag::T_WEED) continue;
                const int tile = y * kag::BOARD + x;
                stats_.suppression_weed_mask[tile >> 6] |=
                    uint64_t{1} << (tile & 63);
                ++stats_.suppression_weed_count;
            }
    }
    action.finalize();
}

}  // namespace kag::agents::two_random_shop_league_v179::base::warm::yarn_125268_robust::detail::residual
