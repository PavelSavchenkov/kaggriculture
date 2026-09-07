#pragma once
#include "../native_semantic_control_v2/controller.hpp"

namespace day_native::portfolio {
struct Options {
    double seconds = 900, fast_seconds = 360, early_seconds = 2;
    int workers = 8;
};
struct Attempt {
    enum class Kind { constructor, repair, semantic, screen, exact, conservation };
    std::string stage;
    Kind kind;
    operations_research::sat::CpSolverStatus status = operations_research::sat::UNKNOWN;
    double seconds = 0;
    std::string error;
    std::vector<semantic::Event> semantic_events;
};
struct Result {
    std::optional<exact::Result> winner;
    std::vector<Attempt> attempts;
    std::string winning_stage;
    double seconds = 0;
    bool accepted() const;
};

// Owns a copy of the validated v3 problem during solving. No route, hint,
// original schedule, JSON, files or subprocesses enter the native core.
Result solve(const day_solver::DayProblem&, const Options& = {});
} // namespace day_native::portfolio
