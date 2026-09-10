#pragma once
#include "course.hpp"

namespace placement {
struct Anchor { int hour, cell; kag::UnitAction action; bool field = false; };
using Anchors = std::array<std::vector<Anchor>, 40>;

inline void spawn_position(std::array<int, 40>& positions, int units) {
    constexpr std::array<int, 4> access{44, 45, 54, 55};
    std::array<int, 4> occupied{};
    for (int u = 0; u < units; ++u)
        for (int s = 0; s < 4; ++s) occupied[s] += positions[u] == access[s];
    positions[units] = access[std::min_element(occupied.begin(), occupied.end()) - occupied.begin()];
}

inline int moved(int cell, int op) {
    if (op == kag::OP_NORTH) return cell - 10;
    if (op == kag::OP_SOUTH) return cell + 10;
    if (op == kag::OP_EAST) return cell + 1;
    if (op == kag::OP_WEST) return cell - 1;
    return cell;
}

inline Anchors source_anchors(const Day& day, const Layout& layout) {
    Anchors anchors;
    std::array<int, 40> positions{}; positions[0] = 44;
    std::array<std::vector<day_solver::TileWorkAction>, 100> work;
    std::array<size_t, 100> cursor{};
    for (const auto& tile : day.problem.tile_work) work[tile.tile] = tile.actions;
    int units = 1;
    for (int h = 0; h < 24; ++h) {
        const auto& action = day.physical[h];
        for (int u = 0; u < units; ++u) {
            const auto operation = action.units[u]; const int cell = positions[u];
            if (operation.op >= kag::OP_NORTH && operation.op <= kag::OP_WEST) {
                positions[u] = moved(cell, operation.op); continue;
            }
            if (operation.op == kag::OP_PASS) continue;
            const bool place = operation.op == kag::OP_PLACE && kag::is_animal(operation.arg) && cursor[cell] < work[cell].size() &&
                work[cell][cursor[cell]].op == kag::OP_PLACE;
            const bool field = place || (operation.op >= kag::OP_PLANT && operation.op <= kag::OP_CARE);
            if (field) {
                if (cursor[cell] >= work[cell].size() || work[cell][cursor[cell]].op != operation.op)
                    throw std::runtime_error("source anchors disagree with tile work");
                ++cursor[cell];
            }
            anchors[u].push_back({h, field ? layout[cell] : cell, operation, field});
        }
        for (int s = 0; s < action.n_orders; ++s) if (action.orders[s].op == kag::M_HIRE) {
            spawn_position(positions, units); ++units;
        }
    }
    return anchors;
}

// Preserve every productive action and its hour, actor and inventory flow.
// Only replace movement/PASS between anchors with Manhattan paths. The final
// strict replay checks the whole physical contract, including hire spawning.
inline std::optional<Schedule> route_repair_anchors(const Day& day, const DayProblem& problem,
                                                   const Anchors& anchors, int variant) {
    Schedule result = day.physical;
    labor::offline::physical_orders(problem, result);
    for (auto& action : result) std::fill(std::begin(action.units), std::end(action.units), kag::UnitAction{});
    std::array<int, 40> positions{}; positions[0] = 44;
    std::array<size_t, 40> cursor{};
    int units = 1;
    for (int h = 0; h < 24; ++h) {
        auto& action = result[h]; action.n_units = units;
        for (int u = 0; u < units; ++u) {
            if (cursor[u] == anchors[u].size()) continue;
            const auto& next = anchors[u][cursor[u]];
            const int cell = positions[u];
            const int dx = next.cell % 10 - cell % 10, dy = next.cell / 10 - cell / 10;
            const int distance = std::abs(dx) + std::abs(dy);
            if (distance > next.hour - h) return std::nullopt;
            if (h == next.hour) { action.units[u] = next.action; ++cursor[u]; continue; }
            if (!distance || ((variant & 2) && distance < next.hour - h)) continue;
            const bool horizontal = dx && (!(variant & 1) || !dy);
            action.units[u].op = horizontal ? (dx > 0 ? kag::OP_EAST : kag::OP_WEST) : (dy > 0 ? kag::OP_SOUTH : kag::OP_NORTH);
            positions[u] = moved(cell, action.units[u].op);
        }
        for (int s = 0; s < action.n_orders; ++s) if (action.orders[s].op == kag::M_HIRE) {
            spawn_position(positions, units); ++units;
        }
        action.finalize();
    }
    const auto replay = day_solver::replay_schedule(problem, result);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty()) return std::nullopt;
    return result;
}

inline std::optional<Schedule> route_repair(const Day& day, const DayProblem& problem,
                                           const Layout& layout, int variant) {
    return route_repair_anchors(day, problem, source_anchors(day, layout), variant);
}
}
