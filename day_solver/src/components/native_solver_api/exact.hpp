#pragma once

#include <cstdint>
#include "internal_hint.hpp"
#include "day_solver_api.hpp"
#include "ortools/sat/cp_model.pb.h"

namespace day_native::exact {
struct HintOptions {
    std::map<HintKind, InternalHint> documents;
    std::set<HintFlag> flags;
    std::set<int> free_routes, free_tasks;
    std::vector<unsigned char> task_domains(int tasks, int workers) const;
};
using Schedule = std::array<kag::Action, day_solver::HOURS>;
struct SolveOptions {
    double seconds = 30;
    int workers = 8;
    bool build_only = false, include_model = false, earliest = false, log_search = false;
};
struct ReplayCheck {
    bool accepted, strict, requirements, invariants;
    std::vector<std::string> errors;
};
struct Result {
    int tasks = 0, workers = 0, variables = 0, constraints = 0, pruned_task_variables = 0;
    double build_seconds = 0, solver_seconds = 0;
    bool solved = false;
    operations_research::sat::CpSolverStatus status = operations_research::sat::UNKNOWN;
    std::int64_t branches = 0, conflicts = 0;
    std::vector<std::string> restrictions;
    std::map<std::string, std::vector<CoreIdentity>> cores;
    std::optional<Schedule> schedule;
    std::optional<ReplayCheck> replay;
    std::optional<operations_research::sat::CpModelProto> model;
};

// All returned fields are owned. No file I/O or JSON is performed by solve.
Result solve(const day_solver::DayProblem& problem, const HintOptions& hints, const SolveOptions& options = {});
} // namespace day_native::exact
