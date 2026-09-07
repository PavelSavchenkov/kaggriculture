#include "controller.hpp"
#include "problem_validation.hpp"
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace day_native::semantic {
namespace sat = operations_research::sat;
using day_semantic::Routes;
using day_semantic::Candidates;
using day_semantic::Count;
using day_semantic::OwnerKey;

bool accepted(const exact::Result& result) {
    return result.solved && (result.status == sat::OPTIMAL || result.status == sat::FEASIBLE) &&
        result.schedule && result.replay && result.replay->accepted;
}
bool cacheable_owner(const exact::Result& result) {
    return result.solved && result.status == sat::INFEASIBLE &&
        result.restrictions == std::vector<std::string>{"fixed_route_owner_type_hint"};
}
bool Result::accepted() const { return exact && semantic::accepted(*exact); }

Backends native_backends() {
    return {
        [](std::string_view, const auto& problem, const auto& hint, const auto& options) {
            return screen::solve(problem, hint, options);
        },
        [](std::string_view, const auto& problem, const auto& hints, const auto& options) {
            return exact::solve(problem, hints, options);
        }, early::native_backends().now};
}

static day_semantic::Hint route_hint(const InternalHint& hint) {
    day_semantic::Hint result;
    result.type_workers = required(hint.type_workers, "type_workers");
    for (const auto& row : hint.assignments)
        result.assignments.push_back({row.task, row.worker, required(row.hour, "hour"), required(row.type, "type")});
    return result;
}
static InternalHint internal_hint(const day_semantic::Hint& hint) {
    InternalHint result;
    result.type_workers = hint.type_workers;
    for (const auto& row : hint.assignments)
        result.assignments.push_back({row.task, row.worker, row.rank, row.type, {}});
    return result;
}
static OwnerKey owner_key(const InternalHint& hint, const std::set<int>& free = {}) {
    return day_semantic::owner_key(route_hint(hint), free);
}
static bool incumbent(const screen::Result& result) {
    return result.solved && (result.status == sat::OPTIMAL || result.status == sat::FEASIBLE) && result.hint;
}
static bool penalties(const screen::Result& result) {
    return !result.deficits.empty() || !result.violations.empty();
}
static std::string number(int value, int width = 2) {
    std::ostringstream stream;
    stream << std::setw(width) << std::setfill('0') << value;
    return stream.str();
}

Session::Session(day_solver::DayProblem problem, Backends backends)
    : problem_(std::move(problem)), backends_(std::move(backends)) {
    const auto issues = day_solver::validate_problem(problem_);
    if (!issues.empty()) throw std::runtime_error(issues[0].path + ": " + issues[0].message);
    if (!backends_.screen || !backends_.exact || !backends_.now) throw std::runtime_error("all semantic backends required");
}

struct Controller {
    Session& session;
    const InternalHint& proposal;
    const Options& options;
    const double started;
    Result result;
    day_semantic::Hint source;
    Routes current;
    std::set<Routes> seen;
    std::set<OwnerKey> screened_owners;
    std::set<std::vector<std::tuple<int, int, int>>> checked_assignments;

    Controller(Session& session, const InternalHint& proposal, const Options& options)
        : session(session), proposal(proposal), options(options), started(session.backends_.now()) {}
    double remaining() const { return options.seconds - (session.backends_.now() - started); }
    double budget(double requested) const { return std::max(0.05, std::min(requested, remaining())); }
    const day_semantic::Context& context() const { return *session.context_; }
    Result finish() { result.seconds = session.backends_.now() - started; return std::move(result); }

    // The old CLI reports model/hint rejection and lets the portfolio continue.
    // Preserve that boundary in process. Public v3 validation stays fail-fast in
    // Session's constructor; a rejected internal proposal is not a day proof.
    exact::Result invoke_exact(const std::string& name, const exact::HintOptions& hints, const exact::SolveOptions& settings) {
        exact::Result report;
        std::string error;
        try { report = session.backends_.exact(name, session.problem_, hints, settings); }
        catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& failure) { report.status = sat::MODEL_INVALID; error = failure.what(); }
        result.events.push_back({name, Event::Kind::exact, report.status, report.solver_seconds, 0, std::move(error)});
        return report;
    }
    screen::Result invoke_screen(const std::string& name, const InternalHint& hint, const screen::SolveOptions& settings) {
        screen::Result report;
        std::string error;
        try { report = session.backends_.screen(name, session.problem_, hint, settings); }
        catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& failure) { report.status = sat::MODEL_INVALID; error = failure.what(); }
        result.events.push_back({name, Event::Kind::screen, report.status, report.total_seconds, 0, std::move(error)});
        return report;
    }

    exact::Result exact_call(const std::string& name, const InternalHint& hint, HintKind kind,
                             double seconds, int workers, const std::set<int>& free = {}) {
        exact::HintOptions hints;
        hints.documents.emplace(kind, hint);
        hints.free_routes = free;
        exact::SolveOptions settings;
        settings.seconds = seconds; settings.workers = workers;
        return invoke_exact(name, hints, settings);
    }
    bool owner_rejected(const InternalHint& hint) const { return session.owner_cache_.contains(owner_key(hint)); }
    exact::Result check_owner(const InternalHint& hint, const std::string& name,
                              const std::set<int>& free = {}, std::optional<double> seconds = {}) {
        const auto key = owner_key(hint, free);
        if (const auto found = session.owner_cache_.find(key); found != session.owner_cache_.end()) {
            auto report = found->second;
            report.solver_seconds = 0;
            result.events.push_back({name, Event::Kind::cached_owner, report.status});
            return report;
        }
        auto report = exact_call(name, hint, HintKind::route_owner,
                                 budget(seconds.value_or(options.exact_seconds)), options.workers, free);
        if (cacheable_owner(report)) session.owner_cache_.emplace(key, report);
        return report;
    }
    screen::Result screen_routes(const Routes& routes, const std::string& name, bool soft = true,
                                  std::optional<bool> fix_profiles = {}, std::optional<double> seconds = {}, int seed = 0) {
        const auto hint = internal_hint(day_semantic::write_hint(routes, source));
        screen::SolveOptions settings;
        settings.seconds = budget(seconds.value_or(soft ? options.screen_seconds : options.hard_screen_seconds));
        settings.workers = soft ? options.workers : 1;
        settings.seed = seed;
        settings.model.dynamic = true; settings.model.fixed_order = true;
        settings.model.fixed_profile = fix_profiles.value_or(options.fix_source_profiles);
        settings.model.soft_precedence = soft; settings.model.soft_availability = soft;
        return invoke_screen(name, hint, settings);
    }
    bool publish(const InternalHint& hint, const std::string& name, exact::Result report,
                 bool fixed = false, const std::set<int>& free = {}) {
        if (!semantic::accepted(report)) return false;
        result.winning_hint = hint; result.winning_call = name;
        result.exact = std::move(report); result.fixed_task_times = fixed; result.free_routes = free;
        return true;
    }
    bool check_assignment(const InternalHint& hint, const std::string& name) {
        if (options.task_time_seconds <= 0 || remaining() <= 0 || owner_rejected(hint)) return false;
        std::vector<std::tuple<int, int, int>> key;
        for (const auto& row : hint.assignments) key.emplace_back(row.task, row.worker, required(row.hour, "hour"));
        std::sort(key.begin(), key.end());
        if (!checked_assignments.insert(std::move(key)).second) return false;
        return publish(hint, name, exact_call(name, hint, HintKind::fixed_partial, budget(options.task_time_seconds), 1), true);
    }
    Candidates candidates(const Routes& routes, const screen::Result& report, int round) {
        Candidates values;
        if (!report.violations.empty()) {
            std::vector<day_semantic::Violation> violations;
            for (const auto& row : report.violations) violations.push_back({row.predecessor, row.successor});
            values = context().semantic_neighbors(routes, violations, options.max_arcs);
        } else {
            std::vector<day_semantic::Deficit> deficits;
            std::vector<day_semantic::Delivery> deliveries;
            for (const auto& row : report.deficits) deficits.push_back({row.item, row.deadline, row.quantity});
            for (const auto& row : report.deliveries)
                if (!row.delivered_by.empty()) deliveries.push_back({row.item, row.route, row.quantity, row.delivered_by});
            values = context().availability_neighbors(routes, deficits, deliveries);
        }
        values = context().rank(values, seen);
        result.events.push_back({"ranked_" + number(round), Event::Kind::candidates, sat::UNKNOWN, 0, values.size()});
        return values;
    }
    auto score(const Routes& routes, const screen::Result& report) const {
        Count deficit = 0, longest = 0, total = 0;
        for (const auto& row : report.deficits) deficit += row.quantity;
        for (const auto& route : routes) { auto length = context().length(route); longest = std::max(longest, length); total += length; }
        return std::tuple{deficit, report.violations.size(), longest, total};
    }

    Result run() {
        const double early_budget = std::min(options.early_seconds, remaining());
        if (early_budget > 0) {
            early::Backends backends{
                [&](const auto&, const auto& hint, const auto& settings) {
                    return invoke_screen("early_completion/screen", hint, settings);
                },
                [&](const auto&, const auto& hints, const auto& settings) {
                    return invoke_exact("early_completion/exact", hints, settings);
                }, session.backends_.now};
            early::Options settings;
            settings.seconds = early_budget; settings.fix_source_profiles = options.fix_source_profiles;
            auto early = early::complete(session.problem_, proposal, settings, backends);
            if (early.accepted()) {
                publish(*early.coarse->hint, "early_completion/exact", std::move(*early.exact), true);
                return finish();
            }
        }
        if (!session.context_) session.context_ = prepare(session.problem_);
        source = route_hint(proposal);
        current = day_semantic::read_routes(source); seen.insert(current);
        auto direct = check_owner(proposal, "direct_owner_exact");
        const int late_route = context().late_input_route(source);
        if (direct.status == sat::INFEASIBLE && late_route >= 0 && options.late_owner_seconds > 0 && remaining() > 1) {
            auto late = check_owner(proposal, "late_input_owner_exact", {late_route}, options.late_owner_seconds);
            if (publish(proposal, "late_input_owner_exact", std::move(late), false, {late_route})) return finish();
        }
        if (semantic::accepted(direct)) { publish(proposal, "direct_owner_exact", std::move(direct)); return finish(); }
        screened_owners.insert(owner_key(proposal));
        auto report = screen_routes(current, "round_00_base");
        if (report.status == sat::UNKNOWN && options.initial_retry_seconds > options.screen_seconds && remaining() > options.screen_seconds)
            report = screen_routes(current, "round_00_retry", true, {}, options.initial_retry_seconds);
        std::optional<InternalHint> winner;
        for (int round = 0; round < options.max_rounds; ++round) {
            if (remaining() <= 0 || !incumbent(report)) break;
            const auto prefix = "round_" + number(round);
            if (!penalties(report) && check_assignment(*report.hint, prefix + "_screen_time_exact")) return finish();
            if (options.screen_owner_seconds > 0 && screened_owners.insert(owner_key(*report.hint)).second) {
                auto owner = check_owner(*report.hint, prefix + "_screen_owner", {}, options.screen_owner_seconds);
                if (publish(*report.hint, prefix + "_screen_owner", std::move(owner))) return finish();
            }
            if (penalties(report) && report.status != sat::OPTIMAL) {
                auto hard = screen_routes(current, prefix + "_hard_probe", false);
                if (incumbent(hard)) { winner = std::move(hard.hint); break; }
            }
            if (!penalties(report)) {
                auto hard = screen_routes(current, prefix + "_hard", false);
                if (incumbent(hard)) winner = std::move(hard.hint);
                break;
            }
            if (!round && options.defer_owner_repair && cacheable_owner(direct)) {
                result.deferred = true; result.exact = std::move(direct); result.deferred_screen = std::move(report);
                return finish();
            }
            auto ranked = candidates(current, report, round);
            std::optional<Routes> best;
            screen::Result best_report;
            for (int index = 0; index < int(ranked.size()) && remaining() > 0; ++index) {
                auto candidate_report = screen_routes(ranked[index], "round_" + number(round + 1) + "_candidate_" + number(index, 3));
                if (!incumbent(candidate_report)) continue;
                const bool zero = !penalties(candidate_report);
                if (!best || score(ranked[index], candidate_report) < score(*best, best_report)) {
                    best = std::move(ranked[index]); best_report = std::move(candidate_report);
                }
                if (zero) break;
            }
            if (!best) break;
            current = std::move(*best); report = std::move(best_report);
            if (!penalties(report)) {
                const auto prefix = "round_" + number(round + 1);
                if (check_assignment(*report.hint, prefix + "_screen_time_exact")) return finish();
                auto hard = screen_routes(current, prefix + "_hard", false);
                if (incumbent(hard)) winner = std::move(hard.hint);
                break;
            }
        }
        const bool soft_partition = !winner && incumbent(report) && !penalties(report);
        if (soft_partition) winner = report.hint;
        if (!winner || remaining() <= 0) return finish();
        const auto selected = *winner;
        if (check_assignment(selected, "winner_time_exact")) return finish();
        bool owner_infeasible = false;
        if (remaining() > 0) {
            auto owner = check_owner(selected, "owner_exact_early");
            owner_infeasible = owner.solved && owner.status == sat::INFEASIBLE;
            if (publish(selected, "owner_exact_early", owner)) return finish();
            result.exact = std::move(owner);
        }
        if (!owner_infeasible && !soft_partition && remaining() > 0) {
            auto ordered = exact_call("exact", selected, HintKind::route_type, budget(options.exact_seconds), options.workers);
            if (publish(selected, "exact", ordered)) return finish();
            result.exact = std::move(ordered);
        }
        for (int seed : {1, 2, 3}) {
            if (remaining() <= 0) break;
            const auto name = "profile_" + number(seed);
            auto profile = screen_routes(current, name, false, {}, {}, seed);
            if (!incumbent(profile) || remaining() <= 0 || owner_rejected(*profile.hint)) continue;
            auto ordered = exact_call(name + "/exact", *profile.hint, HintKind::route_type, budget(options.exact_seconds), options.workers);
            // Retain the hint actually checked, including a changed worker group.
            if (publish(*profile.hint, name + "/exact", ordered)) return finish();
            result.exact = std::move(ordered);
        }
        if (!options.fix_source_profiles && source.type_workers.size() > 1 && remaining() > 0) {
            auto profile = screen_routes(current, "source_profile_hard", false, true);
            if (incumbent(profile) && !owner_rejected(*profile.hint) && remaining() > 0) {
                auto ordered = exact_call("source_profile_hard/exact", *profile.hint, HintKind::route_type, budget(options.exact_seconds), options.workers);
                if (publish(*profile.hint, "source_profile_hard/exact", ordered)) return finish();
                result.exact = std::move(ordered);
            }
        }
        auto late = context().late_placement_neighbors(current);
        result.events.push_back({"late", Event::Kind::candidates, sat::UNKNOWN, 0, late.size()});
        for (int index = 0; index < std::min(8, int(late.size())) && remaining() > 0; ++index) {
            const auto name = "late_place_candidate_" + number(index);
            auto profile = screen_routes(late[index], name, false);
            if (!incumbent(profile) || remaining() <= 0) continue;
            auto ordered = exact_call(name + "/exact", *profile.hint, HintKind::route_type, budget(options.exact_seconds), options.workers);
            if (publish(*profile.hint, name + "/exact", ordered)) return finish();
            result.exact = std::move(ordered);
        }
        return finish();
    }
};

Result Session::repair(const InternalHint& proposal, const Options& options) {
    for (double seconds : {options.seconds, options.early_seconds, options.screen_seconds, options.initial_retry_seconds,
                           options.hard_screen_seconds, options.exact_seconds, options.task_time_seconds,
                           options.screen_owner_seconds, options.late_owner_seconds})
        if (!std::isfinite(seconds) || seconds < 0) throw std::runtime_error("nonnegative finite semantic budgets required");
    if (options.workers <= 0 || options.max_rounds < 0 || options.max_arcs < 0) throw std::runtime_error("invalid semantic search limits");
    return Controller(*this, proposal, options).run();
}
} // namespace day_native::semantic
