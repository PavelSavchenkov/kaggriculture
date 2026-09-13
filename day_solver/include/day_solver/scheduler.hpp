#pragma once
#include "day_solver_api.hpp"
#include <optional>

namespace day_scheduler {
enum class Search { Portfolio, Regret, RegretDeferred, RegretFast };
struct Options {
    double seconds = 900;
    int fallback_workers = 8;
    Search search = Search::Portfolio;
};
struct Stage {
    std::string name;
    std::string status;
    double seconds = 0;
};
struct Result {
    std::optional<std::array<kag::Action, day_solver::HOURS>> schedule;
    double seconds = 0;
    std::vector<Stage> stages;
};

// Public v3 input only. Empty schedule means UNKNOWN; malformed input throws.
// Every returned schedule has passed strict replay. Budget is a soft limit.
Result solve(const day_solver::DayProblem&, const Options& = {});

// For a problem assembled or changed directly in C++: rebuild compatibility
// fields from the public v3 inputs and validate. Does not perform any search.
// Call after edits to purchases, terminal inventories, or end-tile states.
void prepare_problem(day_solver::DayProblem&);
}
