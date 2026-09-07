#include "quick.hpp"
#include "stock_guard.hpp"
#include "../native_stock_screen/screen.hpp"
#include "../native_light_exact/exact.hpp"
#include "../native_light_screen/screen.hpp"
#include "../native_constructor_generic_v2/constructor.hpp"
#include "../native_public_portfolio/internal.hpp"
#include "problem_validation.hpp"
#include "replay.hpp"
#include <chrono>

namespace day_native::quick_v11 {
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
    dc::require(options.repair_steps >= -1 && options.repair_steps <= 100, "invalid repair step budget");
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
    int tasks = 0;
    for (const auto& work : problem.tile_work) tasks += work.actions.size();
    if (remaining() > 0 && tasks <= 12 && problem.worker_count <= 4) {
        exact::SolveOptions settings;
        settings.seconds = std::min(options.completion_seconds, remaining()); settings.workers = 1;
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
        if (options.repair_steps >= 0) {
            // Fixed work budget: do not let a sub-millisecond timing difference
            // choose the route partition. The outer public budget still applies.
            settings.seconds = std::max(0.0, remaining());
            settings.repair.seconds = 0;
            settings.repair.iterations = options.repair_steps;
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
        completion.screen_seconds = std::min(0.3, options.completion_seconds);
        completion.exact_seconds = options.completion_seconds;
        const auto completion_started = now();
        portfolio::Attempt complete{stage + "/complete", portfolio::Attempt::Kind::semantic, sat::UNKNOWN, 0, {}, {}};
        try {
            auto backends = early::native_backends();
            if (options.compact) backends.exact = [&](const auto& p, const auto& h, const auto& s) {
                return light::solve(p, h, s, options.exact_tuning);
            };
            backends.screen = [&](const auto& p, const auto& h, const auto& s) {
                auto first = light_screen::solve(p, h, s, options.screen_tuning);
                if (!first.hint || !violates_purchased_stock(p, *first.hint)) return first;
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
            }
        } catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) { complete.status = sat::MODEL_INVALID; complete.error = error.what(); }
        complete.seconds = now() - completion_started;
        result.attempts.push_back(std::move(complete));
    }
    if (!result.accepted() && options.fallback && remaining() > 0) {
        auto limits = options.reference; limits.seconds = remaining();
        auto reference = portfolio::solve(problem, limits);
        result.attempts.insert(result.attempts.end(), std::make_move_iterator(reference.attempts.begin()),
                               std::make_move_iterator(reference.attempts.end()));
        if (reference.accepted()) keep(std::move(*reference.winner), reference.winning_stage);
    }
    result.seconds = now() - started;
    return result;
}
}
