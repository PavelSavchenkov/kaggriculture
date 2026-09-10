#pragma once
#include "weed_schedule_repair.hpp"

namespace placement {
inline bool preparation(int op) { return op == kag::OP_DIG || op == kag::OP_BUILD_COOP || op == kag::OP_BUILD_PASTURE; }

// Work keys contain cell, pairs of operation/argument, then 255. Drop only
// preparation operations and cells left with no biological work.
inline std::string preparation_key(const std::string& key) {
    std::string result;
    for (size_t i = 0; i < key.size();) {
        const unsigned cell = uint8_t(key[i++]);
        if (cell >= 100) throw std::runtime_error("invalid work-key cell");
        std::string actions;
        while (i < key.size() && uint8_t(key[i]) != 255) {
            if (i + 1 >= key.size()) throw std::runtime_error("truncated work-key operation");
            const int op = uint8_t(key[i]);
            if (!preparation(op)) actions.append(key, i, 2);
            i += 2;
        }
        if (i == key.size()) throw std::runtime_error("unterminated work-key cell");
        ++i;
        if (!actions.empty()) { result.push_back(char(cell)); result += actions; result.push_back(char(255)); }
    }
    return result;
}

// Retain all non-preparation field actions and their original times. Remove
// obsolete digging/building, then insert the target preparation between those
// anchors. It is a bounded proposal; callers must replay the complete contract.
inline std::optional<Schedule> repair_preparation_schedule(const DayProblem& p, Schedule schedule, int variant, int last_work_hour = 23) {
    struct Field { int hour, worker, op, arg; };
    std::array<std::vector<Field>, 100> anchors;
    std::array<std::vector<day_solver::TileWorkAction>, 100> target;
    for (const auto& work : p.tile_work) {
        const auto& tile = p.start.managed_tiles[work.tile]; target[tile.y * 10 + tile.x] = work.actions;
    }
    auto frames = schedule_positions(schedule);
    for (int h = 0; h < 24; ++h) for (int u = 0; u < schedule[h].n_units; ++u) {
        auto& action = schedule[h].units[u]; const int cell = frames[h][u];
        if (cell < 0 || cell >= 100) return std::nullopt;
        if (preparation(action.op)) { action = {}; continue; }
        const bool animal_place = action.op == kag::OP_PLACE && kag::is_animal(action.arg) &&
            std::any_of(target[cell].begin(), target[cell].end(), [&](const auto& a) { return a.op == kag::OP_PLACE && a.arg == action.arg; });
        if (animal_place || (action.op >= kag::OP_PLANT && action.op <= kag::OP_CARE))
            anchors[cell].push_back({h, u, action.op, action.arg});
    }
    auto insert = [&](int cell, int op, int first, int last) {
        if (first > last) return false;
        frames = schedule_positions(schedule);
        for (int i = 0; i <= last - first; ++i) {
            const int h = variant == 1 ? first + i : last - i;
            for (int u = 0; u < schedule[h].n_units; ++u) if (frames[h][u] == cell && schedule[h].units[u].op == kag::OP_PASS) {
                schedule[h].units[u] = {uint8_t(op), 0, 0}; return true;
            }
        }
        if (variant == 0) return false;
        // A spare worker may make a closed trip, provided hiring cannot change
        // its spawn consequences while it is away from the original position.
        for (int h = 0; h <= last; ++h) for (int u = 0; u < schedule[h].n_units; ++u) {
            const int origin = frames[h][u], distance = std::abs(cell % 10 - origin % 10) + std::abs(cell / 10 - origin / 10);
            const int length = 2 * distance + 1, action_hour = h + distance;
            if (action_hour < first || action_hour > last || h + length > 24) continue;
            bool idle = true;
            for (int t = h; t < h + length; ++t) {
                idle &= u < schedule[t].n_units && schedule[t].units[u].op == kag::OP_PASS;
                if (t + 1 < h + length) for (int s = 0; s < schedule[t].n_orders; ++s) idle &= schedule[t].orders[s].op != kag::M_HIRE;
            }
            if (!idle) continue;
            int t = h, at = origin;
            auto walk = [&](int destination) {
                while (at % 10 != destination % 10) {
                    const int move = at % 10 < destination % 10 ? kag::OP_EAST : kag::OP_WEST;
                    schedule[t++].units[u] = {uint8_t(move), 0, 0}; at = moved(at, move);
                }
                while (at / 10 != destination / 10) {
                    const int move = at / 10 < destination / 10 ? kag::OP_SOUTH : kag::OP_NORTH;
                    schedule[t++].units[u] = {uint8_t(move), 0, 0}; at = moved(at, move);
                }
            };
            walk(cell); schedule[t++].units[u] = {uint8_t(op), 0, 0}; walk(origin); return true;
        }
        return false;
    };
    for (int cell = 0; cell < 100; ++cell) {
        std::vector<std::vector<int>> gaps(anchors[cell].size() + 1);
        size_t cursor = 0;
        for (const auto& action : target[cell]) {
            if (preparation(action.op)) { gaps[cursor].push_back(action.op); continue; }
            if (cursor >= anchors[cell].size()) return std::nullopt;
            const auto& anchor = anchors[cell][cursor++];
            if (action.op != anchor.op || ((action.op == kag::OP_PLACE || action.op == kag::OP_PLANT) && action.arg != anchor.arg)) return std::nullopt;
        }
        if (cursor != anchors[cell].size()) return std::nullopt;
        for (size_t gap = 0; gap < gaps.size(); ++gap) {
            const int first = gap ? anchors[cell][gap - 1].hour + 1 : 0;
            int last = gap < anchors[cell].size() ? std::min(last_work_hour, anchors[cell][gap].hour - 1) : last_work_hour;
            for (auto it = gaps[gap].rbegin(); it != gaps[gap].rend(); ++it) {
                if (!insert(cell, *it, first, last)) return std::nullopt;
                // Locate the inserted preparation to preserve its target order.
                frames = schedule_positions(schedule);
                int inserted = -1;
                for (int h = first; h <= last; ++h) for (int u = 0; u < schedule[h].n_units; ++u)
                    if (frames[h][u] == cell && schedule[h].units[u].op == *it) inserted = std::max(inserted, h);
                if (inserted < 0) throw std::runtime_error("lost inserted preparation");
                last = inserted - 1;
            }
        }
    }
    for (auto& action : schedule) action.finalize();
    return schedule;
}
}
