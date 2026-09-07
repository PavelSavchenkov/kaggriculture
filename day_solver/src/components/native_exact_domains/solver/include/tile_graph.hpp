#pragma once

#include <array>
#include <vector>

#include "day_solver_api.hpp"

namespace day_solver {

struct TileNode {
    int hour = 0;
    int prefix = 0;
    ManagedTileState state;
};

struct TileArc {
    int from = 0;
    int to = 0;
    int prefix = 0;
};

struct TileGraph {
    int tile = 0;
    int task_count = 0;
    std::vector<TileNode> nodes;
    std::vector<TileArc> arcs;
    std::vector<int> terminals;
};

std::vector<TileGraph> build_tile_graphs(const DayProblem& problem);

}  // namespace day_solver

