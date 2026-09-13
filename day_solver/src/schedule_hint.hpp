#pragma once
#include "storage.hpp"
#include "components/native_solver_api/internal_hint.hpp"

namespace day_scheduler {
inline day_native::InternalHint schedule_hint(
        const day_solver::DayProblem& problem,
        const std::array<kag::Action, day_solver::HOURS>& actions) {
    const auto frames = storage::trace(problem, actions);
    std::array<std::vector<std::pair<int, const day_solver::TileWorkAction*>>, kag::BOARD * kag::BOARD> at;
    std::array<int, kag::BOARD * kag::BOARD> prefix{};
    int count = 0;
    for (const auto& work : problem.tile_work) {
        const auto& tile = problem.start.managed_tiles[work.tile];
        for (const auto& action : work.actions)
            at[tile.x + kag::BOARD * tile.y].push_back({count++, &action});
    }
    day_native::InternalHint hint;
    for (int hour = 0; hour < day_solver::HOURS; ++hour)
        for (int worker = 0; worker < int(frames[hour].workers.size()); ++worker) {
            const auto& state = frames[hour].workers[worker];
            const auto& action = actions[hour].units[worker];
            const int cell = state.x + kag::BOARD * state.y;
            if (prefix[cell] == int(at[cell].size())) continue;
            const auto& [id, expected] = at[cell][prefix[cell]];
            if (action.op != expected->op ||
                ((action.op == kag::OP_PLACE || action.op == kag::OP_PLANT) && action.arg != expected->arg)) continue;
            hint.assignments.push_back({id, worker, hour, 0, {}});
            ++prefix[cell];
        }
    if (int(hint.assignments.size()) != count) throw std::runtime_error("schedule hint lost farm tasks");
    hint.type_workers.emplace(1);
    for (int worker = 0; worker < problem.worker_count; ++worker)
        hint.type_workers->front().push_back(worker);
    return hint;
}
}
