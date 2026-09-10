#pragma once
#include "route_repair.hpp"

namespace placement {
using PositionFrames = std::array<std::array<int, 40>, 24>;
inline PositionFrames schedule_positions(const Schedule& schedule) {
    PositionFrames frames; std::array<int, 40> positions{}; positions[0] = 44; int units = 1;
    for (int h = 0; h < 24; ++h) {
        frames[h] = positions;
        for (int u = 0; u < units; ++u) positions[u] = moved(positions[u], schedule[h].units[u].op);
        for (int s = 0; s < schedule[h].n_orders; ++s) if (schedule[h].orders[s].op == kag::M_HIRE) { spawn_position(positions, units); ++units; }
    }
    return frames;
}

// A bounded proposal for a previously valid route with newly required weed
// clearing. It never returns a certificate itself; the complete new contract
// is replayed by the caller after all edits, including terminal deadlines.
inline std::optional<Schedule> repair_weed_schedule(const DayProblem& p, Schedule schedule, int variant) {
    for (const auto& work : p.tile_work) {
        const auto& tile = p.start.managed_tiles[work.tile];
        if (tile.state.kind != day_solver::ManagedTileKind::WEED || work.actions.empty() || work.actions.front().op != kag::OP_DIG) continue;
        const int target = tile.y * 10 + tile.x;
        const auto positions = schedule_positions(schedule);
        int deadline = 24, actor = -1;
        for (int h = 0; h < 24 && deadline == 24; ++h) for (int u = 0; u < schedule[h].n_units; ++u) {
            const int op = schedule[h].units[u].op;
            if (positions[h][u] == target && (op == kag::OP_PLANT || op == kag::OP_BUILD_COOP || op == kag::OP_BUILD_PASTURE)) {
                deadline = h; actor = u; break;
            }
        }
        if (actor < 0) return std::nullopt;
        bool inserted = false;
        for (int h = deadline - 1; h >= 0 && !inserted; --h) for (int u = 0; u < schedule[h].n_units; ++u)
            if (positions[h][u] == target && schedule[h].units[u].op == kag::OP_PASS) {
                schedule[h].units[u] = {kag::OP_DIG, 0, 0}; inserted = true; break;
            }
        if (!inserted && variant != 1) {
            for (int h = deadline - 1; h >= 0 && !inserted; --h) for (int u = 0; u < schedule[h].n_units && !inserted; ++u) {
                const int origin = positions[h][u], dx = target % 10 - origin % 10, dy = target / 10 - origin / 10;
                const int distance = std::abs(dx) + std::abs(dy), length = 2 * distance + 1;
                if (h + distance >= deadline || h + length > 24) continue;
                bool idle = true;
                for (int t = h; t < h + length; ++t) {
                    idle &= u < schedule[t].n_units && schedule[t].units[u].op == kag::OP_PASS;
                    if (t + 1 < h + length) for (int s = 0; s < schedule[t].n_orders; ++s) idle &= schedule[t].orders[s].op != kag::M_HIRE;
                }
                if (!idle) continue;
                int t = h, cell = origin;
                auto walk = [&](int destination) {
                    while (cell % 10 != destination % 10) {
                        const int op = cell % 10 < destination % 10 ? kag::OP_EAST : kag::OP_WEST;
                        schedule[t++].units[u] = {uint8_t(op), 0, 0}; cell = moved(cell, op);
                    }
                    while (cell / 10 != destination / 10) {
                        const int op = cell / 10 < destination / 10 ? kag::OP_SOUTH : kag::OP_NORTH;
                        schedule[t++].units[u] = {uint8_t(op), 0, 0}; cell = moved(cell, op);
                    }
                };
                walk(target); schedule[t++].units[u] = {kag::OP_DIG, 0, 0}; walk(origin); inserted = true;
            }
        }
        if (!inserted && variant != 0) {
            int spare = deadline + 1;
            while (spare < 24 && schedule[spare].units[actor].op != kag::OP_PASS) ++spare;
            if (spare < 24) {
                for (int h = spare; h > deadline; --h) schedule[h].units[actor] = schedule[h - 1].units[actor];
                schedule[deadline].units[actor] = {kag::OP_DIG, 0, 0}; inserted = true;
            }
        }
        if (!inserted) return std::nullopt;
    }
    for (auto& action : schedule) action.finalize();
    return schedule;
}
}
