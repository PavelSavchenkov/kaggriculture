#include "quick.hpp"
#include "stock_guard.hpp"
#include "../native_quick_portfolio_v11/quick.hpp"
#include "../native_stock_screen/screen.hpp"
#include "../native_unnamed_exact/exact.hpp"
#include "../native_ready_screen/screen.hpp"
#include "../native_materialized_screen/screen.hpp"
#include "../native_geometry_screen/screen.hpp"
#include "../native_constructor_generic_v2/constructor.hpp"
#include "../native_public_portfolio/internal.hpp"
#include "problem_validation.hpp"
#include "replay.hpp"
#include <chrono>

namespace day_native::quick_v30 {
namespace dc = day_constructor;
namespace ds = day_solver;
namespace sat = operations_research::sat;
static double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
portfolio::Result solve(const ds::DayProblem& problem, const Options& options) {
    dc::require(problem.format_version == 3, "public scheduler requires v3");
    const auto issues = ds::validate_problem(problem);
    dc::require(issues.empty(), issues.empty() ? "" : issues.front().path + ": " + issues.front().message);
    dc::require(options.rounds >= 1 && options.rounds <= 4 && options.iterations <= 100000,
                "invalid quick round count");
    dc::require(options.exact_tuning >= 0 && options.exact_tuning <= 2 &&
                options.screen_tuning >= 0 && options.screen_tuning <= 2 && options.reference.workers > 0,
                "invalid backend settings");
    dc::require(std::isfinite(options.reference.seconds) && options.reference.seconds >= 0,
                "nonnegative finite total budget required");
    dc::require(options.iterations > 0 && options.variants >= 0 && options.variants <= 8,
                "invalid quick search counts");
    dc::require(std::isfinite(options.route_seconds) && options.route_seconds > 0 &&
                std::isfinite(options.completion_seconds) && options.completion_seconds > 0,
                "positive finite quick budgets required");
    dc::require(std::isfinite(options.retry_seconds) && options.retry_seconds >= 0, "invalid fixed-work retry budget");
    dc::require(std::isfinite(options.screen_seconds) && options.screen_seconds > 0, "positive finite coarse budget required");
    dc::require(std::isfinite(options.geometry_seconds) && options.geometry_seconds >= 0 &&
                options.geometry_after >= 1 && options.geometry_after <= 16, "invalid geometry retry settings");
    std::optional<unnamed::Session> exact_session;
    using RouteKey = std::pair<std::vector<std::vector<int>>, std::vector<std::array<int, 4>>>;
    std::set<RouteKey> saved;
    std::vector<std::pair<std::string, InternalHint>> timing_retries;
    auto remember = [&](const std::string& stage, const InternalHint& hint) {
        RouteKey key{hint.type_workers.value_or(std::vector<std::vector<int>>{}), {}};
        for (const auto& row : hint.assignments)
            key.second.push_back({row.task, row.worker, required(row.hour, "hour"), row.type.value_or(-1)});
        std::sort(key.second.begin(), key.second.end());
        if (saved.insert(std::move(key)).second) timing_retries.push_back({stage, hint});
    };
    portfolio::Result result;
    const double started = now();
    auto remaining = [&] { return options.reference.seconds - (now() - started); };
    auto keep = [&](exact::Result value, const std::string& stage) {
        if (!semantic::accepted(value)) return false;
        const auto replay = ds::replay_schedule(problem, *value.schedule);
        dc::require(replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied,
                    "quick final replay rejected schedule");
        result.winner = std::move(value); result.winning_stage = stage;
        return true;
    };
    double timing_used = 0;
    std::size_t timing_next = 0;
    auto try_timing = [&] {
      while (timing_next < timing_retries.size()) {
        const double left = std::min(options.geometry_seconds - timing_used, remaining());
        if (result.accepted() || left <= 0) break;
        const auto& [source_stage, hint] = timing_retries[timing_next++];
        const std::string stage = "geometry_retry/" + source_stage;
        const auto before = now();
        portfolio::Attempt event{stage, portfolio::Attempt::Kind::semantic, sat::UNKNOWN, 0, {}, {}};
        try {
            early::Options limits;
            limits.seconds = std::min({0.9, options.completion_seconds, left});
            limits.screen_seconds = std::min({0.3, options.screen_seconds, limits.seconds});
            limits.exact_seconds = 0.6;
            auto backends = early::native_backends();
            if (options.compact) backends.exact = [&](const auto& p, const auto& h, const auto& settings) {
                const bool prepared_now = !exact_session;
                if (prepared_now) exact_session.emplace(p);
                auto solved = exact_session->solve(h, settings, options.exact_tuning);
                if (prepared_now) solved.build_seconds += exact_session->preparation_seconds();
                return solved;
            };
            backends.screen = [&](const auto& p, const auto& h, const auto& settings) {
                auto first = geometry_screen::solve(p, h, settings, options.screen_tuning);
                if (!first.hint || !violates_purchased_stock(p, *first.hint)) return first;
                event.semantic_events.push_back({"screen_before_stock_bounds", semantic::Event::Kind::screen,
                    first.status, first.total_seconds});
                const double rest = settings.seconds - first.total_seconds;
                if (rest <= 0) { first.status = sat::UNKNOWN; first.hint.reset(); return first; }
                auto retry = settings; retry.seconds = rest;
                auto guarded = stock_screen::solve(p, h, retry, options.screen_tuning);
                guarded.total_seconds += first.total_seconds;
                return guarded;
            };
            auto value = early::complete(problem, hint, limits, backends);
            if (value.coarse) event.semantic_events.push_back({"screen", semantic::Event::Kind::screen,
                value.coarse->status, value.coarse->total_seconds});
            if (value.exact) event.semantic_events.push_back({"exact", semantic::Event::Kind::exact,
                value.exact->status, value.exact->solver_seconds + value.exact->build_seconds});
            if (value.accepted()) {
                event.status = sat::FEASIBLE;
                keep(std::move(*value.exact), stage);
            }
        } catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) { event.status = sat::MODEL_INVALID; event.error = error.what(); }
        event.seconds = now() - before;
        timing_used += event.seconds;
        result.attempts.push_back(std::move(event));
      }
    };
    int tasks = 0;
    for (const auto& work : problem.tile_work) tasks += work.actions.size();
    if (remaining() > 0 && tasks <= 12 && problem.worker_count <= 4) {
        exact::SolveOptions settings;
        settings.seconds = std::min(0.6, remaining()); settings.workers = 1;
        const auto before = now();
        auto value = exact::solve(problem, {}, settings);
        result.attempts.push_back({"quick/initial_exact", portfolio::Attempt::Kind::exact, value.status, now() - before, {}, {}});
        keep(std::move(value), "quick/initial_exact");
    }
    for (int round = 0; round < options.rounds && !result.accepted(); ++round)
    for (int variant = 0; variant < options.variants && !result.accepted(); ++variant) {
        if (remaining() <= 0) break;
        const int mode = variant % 4;
        dc::ConstructorOptions settings;
        settings.iterations = options.iterations * (1 << (2 * round));
        settings.seconds = settings.repair.seconds = std::min(options.route_seconds, remaining());
        settings.routing.shared_seed_flow = variant >= 4;
        if (mode == 1) {
            settings.routing.shared_resource_flow = true;
            settings.repair.iterations = 30; settings.repair.shortlist = 16;
            settings.repair.plateau_steps = 12; settings.repair.optimize_balance = true;
        } else if (mode == 2) {
            settings.repair.shortlist = 16; settings.repair.plateau_steps = 12;
            settings.repair.optimize_balance = true;
        } else if (mode == 3) {
            settings.routing.seed = 3; settings.routing.pair_jitter = 200;
            settings.routing.separate_delivery_routes = false;
            settings.repair.optimize_balance = true;
        }
        const std::string stage = "quick/r" + std::to_string(round) + "/v" + std::to_string(variant);
        const auto before = now();
        portfolio::Attempt event{stage + "/construct", portfolio::Attempt::Kind::constructor, sat::UNKNOWN, 0, {}, {}};
        dc::ConstructorResult routes;
        try { routes = dc::construct(problem, settings); }
        catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) { event.status = sat::MODEL_INVALID; event.error = error.what(); }
        if (routes.proposal) event.status = sat::FEASIBLE;
        event.seconds = now() - before;
        result.attempts.push_back(std::move(event));
        if (!routes.proposal) continue;
        InternalHint hint;
        hint.type_workers = routes.proposal->type_workers;
        for (const auto& row : routes.proposal->assignments)
            hint.assignments.push_back({row.task, row.worker, row.rank, row.type, {}});
        early::Options completion;
        if (remaining() <= 0) break;
        completion.seconds = std::min(options.completion_seconds, remaining());
        completion.screen_seconds = std::min(options.screen_seconds, options.completion_seconds);
        completion.exact_seconds = 0.6;
        const auto completion_started = now();
        portfolio::Attempt complete{stage + "/complete", portfolio::Attempt::Kind::semantic, sat::UNKNOWN, 0, {}, {}};
        try {
            std::optional<exact::Result> direct_completion;
            auto backends = early::native_backends();
            if (options.compact || options.materialize) backends.exact = [&](const auto& p, const auto& h, const auto& s) {
                if (direct_completion) return std::move(*direct_completion);
                if (!options.compact) return exact::solve(p, h, s);
                const bool prepared_now = !exact_session;
                if (prepared_now) exact_session.emplace(p);
                auto solved = exact_session->solve(h, s, options.exact_tuning);
                if (prepared_now) solved.build_seconds += exact_session->preparation_seconds();
                return solved;
            };
            backends.screen = [&](const auto& p, const auto& h, const auto& s) {
                screen::Result first;
                if (options.materialize) {
                    auto constructed = materialized_screen::solve(p, h, s, options.screen_tuning);
                    if (constructed.solved && constructed.hint) {
                        complete.semantic_events.push_back({"direct_schedule", semantic::Event::Kind::exact,
                            constructed.schedule ? sat::FEASIBLE : sat::UNKNOWN, constructed.materialize_seconds,
                            0, constructed.materialize_rejection});
                    }
                    if (constructed.schedule) {
                        direct_completion.emplace();
                        direct_completion->solved = true;
                        direct_completion->status = sat::FEASIBLE;
                        direct_completion->schedule = std::move(constructed.schedule);
                        direct_completion->replay = exact::ReplayCheck{true, true, true, true, {}};
                        direct_completion->restrictions = {"strictly_materialized_coarse_schedule"};
                        direct_completion->workers = p.worker_count;
                    }
                    first = std::move(static_cast<screen::Result&>(constructed));
                } else first = ready_screen::solve(p, h, s, options.screen_tuning);
                if (direct_completion || !first.hint || !violates_purchased_stock(p, *first.hint)) return first;
                complete.semantic_events.push_back({"screen_before_stock_bounds", semantic::Event::Kind::screen,
                    first.status, first.total_seconds});
                const double left = s.seconds - first.total_seconds;
                if (left <= 0) { first.status = sat::UNKNOWN; first.hint.reset(); return first; }
                auto retry = s; retry.seconds = left;
                auto guarded = stock_screen::solve(p, h, retry, options.screen_tuning);
                guarded.total_seconds += first.total_seconds;
                return guarded;
            };
            auto value = early::complete(problem, hint, completion, backends);
            if (value.coarse) complete.semantic_events.push_back({"screen", semantic::Event::Kind::screen,
                value.coarse->status, value.coarse->total_seconds});
            if (value.exact) complete.semantic_events.push_back({"exact", semantic::Event::Kind::exact,
                value.exact->status, value.exact->solver_seconds + value.exact->build_seconds});
            if (value.accepted()) {
                complete.status = sat::FEASIBLE;
                keep(std::move(*value.exact), stage + "/complete");
            } else if (options.geometry_seconds > 0 && value.coarse && value.coarse->hint &&
                       value.coarse->total_seconds <= 0.2 && value.exact && value.exact->status == sat::INFEASIBLE)
                remember(stage, hint);
        } catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) { complete.status = sat::MODEL_INVALID; complete.error = error.what(); }
        complete.seconds = now() - completion_started;
        result.attempts.push_back(std::move(complete));
        if (int(timing_retries.size()) >= options.geometry_after) try_timing();
    }
    if (!result.accepted() && options.variants > 0 && options.retry_seconds > 0 && remaining() > 0) {
        quick_v11::Options retry;
        retry.iterations = 64; retry.variants = 8; retry.rounds = 1; retry.repair_steps = 12;
        retry.fallback = false; retry.compact = options.compact;
        retry.exact_tuning = options.exact_tuning; retry.screen_tuning = options.screen_tuning;
        retry.completion_seconds = 0.6;
        retry.reference = options.reference;
        retry.reference.seconds = std::min(options.retry_seconds, remaining());
        auto alternate = quick_v11::solve(problem, retry);
        for (auto& attempt : alternate.attempts) attempt.stage = "fixed_work/" + attempt.stage;
        result.attempts.insert(result.attempts.end(), std::make_move_iterator(alternate.attempts.begin()),
                               std::make_move_iterator(alternate.attempts.end()));
        if (alternate.accepted()) keep(std::move(*alternate.winner), "fixed_work/" + alternate.winning_stage);
    }
    try_timing();
    if (!result.accepted() && options.fallback && remaining() > 0) {
        auto limits = options.reference; limits.seconds = remaining();
        portfolio::Result reference;
        if (options.modern_fallback && options.compact) {
            auto exact_backend = [&](std::string_view, const auto& p, const auto& h, const auto& settings) {
                const bool prepared_now = !exact_session;
                if (prepared_now) exact_session.emplace(p);
                auto solved = exact_session->solve(h, settings, options.exact_tuning);
                if (prepared_now) solved.build_seconds += exact_session->preparation_seconds();
                return solved;
            };
            auto semantic_backends = semantic::native_backends();
            semantic_backends.exact = exact_backend;
            semantic::Session repair_session(problem, std::move(semantic_backends));
            portfolio::detail::Backends backends;
            backends.exact = exact_backend;
            backends.semantic = [&](std::string_view, const auto& hint, const auto& settings) {
                return repair_session.repair(hint, settings);
            };
            reference = portfolio::detail::run(problem, limits, std::move(backends));
        } else reference = portfolio::solve(problem, limits);
        result.attempts.insert(result.attempts.end(), std::make_move_iterator(reference.attempts.begin()),
                               std::make_move_iterator(reference.attempts.end()));
        if (reference.accepted()) keep(std::move(*reference.winner), reference.winning_stage);
    }
    result.seconds = now() - started;
    return result;
}
}
