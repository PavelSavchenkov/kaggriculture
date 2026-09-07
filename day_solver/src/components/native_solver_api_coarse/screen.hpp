#pragma once

#include <cstdint>
#include "../native_solver_api/internal_hint.hpp"
#include "day_solver_api.hpp"
#include "ortools/sat/cp_model.pb.h"

namespace day_native::screen {
using Count = day_solver::InventoryCount;
using Point = std::array<int, 2>;
using Key = std::pair<int, int>;
struct ScreenOptions {
    bool dynamic = false, ignore_availability = false, ignore_pickups = false;
    bool ignore_precedence = false, soft_precedence = false, soft_availability = false;
    bool fixed_order = false, fixed_profile = false, fixed_availability = false;
    std::set<int> free_order;
};
struct Task {
    int id, tile, predecessor, input, crop, output, op;
    Count quantity;
    Point point;
};
struct Profile {
    std::vector<Point> starts;
    int release;
    bool operator==(const Profile&) const = default;
};
struct Violation {
    int predecessor, successor, predecessor_route, successor_route;
    Count predecessor_hour, successor_hour;
};
struct Deficit { int item, deadline; std::optional<int> task; Count quantity; };
struct Delivery {
    int task, item, route;
    Count hour, quantity, delivered;
    std::map<int, Count> delivered_by;
};
struct DataSnapshot {
    std::vector<Task> tasks;
    std::vector<int> early, late, deadlines, owner, route_ids, worker_group;
    std::vector<std::pair<Key, Count>> requirements;
    std::map<Key, std::vector<int>> deliveries;
    std::map<Key, Count> net;
    std::vector<Profile> workers, profiles;
    std::vector<std::vector<int>> groups;
};
struct SolveOptions {
    ScreenOptions model;
    double seconds = 30;
    int workers = 16, seed = 0;
    bool build_only = false, include_model = false, include_data = false;
};
struct Result {
    bool solved = false, initial_spawns_coupled = false, hire_checkpoints_coupled = false;
    operations_research::sat::CpSolverStatus status = operations_research::sat::UNKNOWN;
    double build_seconds = 0, solver_seconds = 0, total_seconds = 0;
    int routes = 0, variables = 0, constraints = 0;
    std::int64_t branches = 0, conflicts = 0;
    std::map<int, std::map<int, std::vector<int>>> pickup_requirements;
    std::optional<std::vector<Profile>> worker_start_profiles;
    std::vector<Violation> violations;
    std::vector<Deficit> deficits;
    std::vector<Delivery> deliveries;
    std::optional<InternalHint> hint;
    std::optional<DataSnapshot> data;
    std::optional<operations_research::sat::CpModelProto> model;
};

// A coarse result is an internal relaxation, never a validated day schedule.
// All fields are owned. No file I/O or JSON occurs in solve.
Result solve(const day_solver::DayProblem& problem, const InternalHint& hint, const SolveOptions& options = {});
} // namespace day_native::screen
