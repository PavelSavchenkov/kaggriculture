#include "tile_graph.hpp"

#include <cassert>
#include <map>
#include <tuple>

// Generated from the unchanged replay source by prepare_tile_graph_adapter.py. Compile
// this translation unit instead of src/replay.cpp, never alongside it.
#include "../build/replay_tile_adapter.cpp"

namespace day_solver {
namespace {

auto state_key(int prefix, const ManagedTileState& s) {
    return std::tuple(prefix, s.kind, s.crop, s.animal, s.age_days,
                      s.stored_units, s.consecutive_dry_days,
                      s.pending_care_bonus, s.fertilizer_days_remaining,
                      s.watered_today, s.fed_today, s.cared_today,
                      s.fertilizer_available);
}

}  // namespace

std::vector<TileGraph> build_tile_graphs(const DayProblem& problem) {
    assert(problem.format_version == 3);
    std::array<int, HOURS> active;
    active.fill(1);
    for (const auto& event : problem.market_plan)
        if (event.market_op == kag::M_HIRE)
            for (int hour = event.hour + 1; hour < HOURS; ++hour)
                active[hour] += event.quantity;
    std::vector<TileGraph> result;
    for (size_t tile = 0; tile < problem.start.managed_tiles.size(); ++tile) {
        DayProblem local = problem;
        local.start.managed_tiles = {problem.start.managed_tiles[tile]};
        local.tile_work = {{0, {}}};
        for (const auto& work : problem.tile_work)
            if (work.tile == static_cast<int>(tile))
                local.tile_work[0].actions = work.actions;
        const ManagedTileState* required = nullptr;
        for (const auto& end : problem.required_end_tiles)
            if (end.tile == static_cast<int>(tile) && end.exact_state)
                required = &*end.exact_state;
        assert(required != nullptr);
        ReplayMachine machine(local);
        TileGraph graph;
        graph.tile = tile;
        graph.task_count = local.tile_work[0].actions.size();
        graph.nodes.push_back({0, 0, local.start.managed_tiles[0].state});
        std::vector<int> layer{0};
        for (int hour = 0; hour < HOURS && !layer.empty(); ++hour) {
            std::map<decltype(state_key(0, {})), int> seen;
            std::vector<int> next;
            for (int source : layer) {
                // Copy before appending nodes, which can reallocate the vector.
                const TileNode node = graph.nodes[source];
                const int maximum = std::min(active[hour], graph.task_count - node.prefix);
                for (int count = 0; count <= maximum; ++count) {
                    const auto state = machine.local_hour(node.state, node.prefix, count, hour);
                    if (!state) continue;
                    const int prefix = node.prefix + count;
                    const auto key = state_key(prefix, *state);
                    auto [position, inserted] = seen.emplace(key, graph.nodes.size());
                    if (inserted) {
                        next.push_back(position->second);
                        graph.nodes.push_back({hour + 1, prefix, *state});
                    }
                    graph.arcs.push_back({source, position->second, prefix});
                }
            }
            layer = std::move(next);
        }
        for (int node : layer)
            if (graph.nodes[node].hour == HOURS &&
                graph.nodes[node].prefix == graph.task_count &&
                graph.nodes[node].state == *required)
                graph.terminals.push_back(node);
        result.push_back(std::move(graph));
    }
    return result;
}

}  // namespace day_solver

