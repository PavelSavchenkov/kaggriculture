#pragma once
#include "../native_early_completion/completion.hpp"
#include "../native_semantic_context/context.hpp"
#include <string_view>

namespace day_native::semantic {
struct Options {
    double seconds = 60, early_seconds = 0, screen_seconds = 3;
    double initial_retry_seconds = 15, hard_screen_seconds = 10, exact_seconds = 20;
    double task_time_seconds = 3, screen_owner_seconds = 0, late_owner_seconds = 10;
    int workers = 8, max_rounds = 4, max_arcs = 4;
    bool fix_source_profiles = false, defer_owner_repair = false;
};
struct Backends {
    std::function<screen::Result(std::string_view, const day_solver::DayProblem&,
                               const InternalHint&, const screen::SolveOptions&)> screen;
    std::function<exact::Result(std::string_view, const day_solver::DayProblem&,
                              const exact::HintOptions&, const exact::SolveOptions&)> exact;
    std::function<double()> now;
};
Backends native_backends();
bool accepted(const exact::Result&);
bool cacheable_owner(const exact::Result&);

struct Event {
    enum class Kind { screen, exact, cached_owner, candidates };
    std::string name;
    Kind kind;
    operations_research::sat::CpSolverStatus status = operations_research::sat::UNKNOWN;
    double seconds = 0;
    std::size_t count = 0;
    std::string backend_error{};
};
struct Result {
    std::optional<exact::Result> exact;
    std::optional<InternalHint> winning_hint;
    std::optional<screen::Result> deferred_screen;
    std::vector<Event> events;
    std::set<int> free_routes;
    std::string winning_call;
    double seconds = 0;
    bool deferred = false, fixed_task_times = false;
    bool accepted() const;
};

// One session belongs to one immutable public v3 input and one backend.
// Only our own constructor/repair proposals enter repair(). No replay route.
// The owner cache is private and survives strong/shared/deferred attempts.
class Session {
public:
    explicit Session(day_solver::DayProblem, Backends = native_backends());
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Result repair(const InternalHint&, const Options& = {});
private:
    friend struct Controller;
    const day_solver::DayProblem problem_;
    const Backends backends_;
    std::optional<day_semantic::Context> context_;
    std::map<day_semantic::OwnerKey, exact::Result> owner_cache_;
};
} // namespace day_native::semantic
