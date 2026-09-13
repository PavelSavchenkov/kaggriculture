#include "quick.hpp"
#include "stock_guard.hpp"
#include "../native_quick_portfolio_v11/quick.hpp"
#include "../native_stock_screen/screen.hpp"
#include "../native_unnamed_exact/exact.hpp"
#include "../native_ready_screen/screen.hpp"
#include "../native_materialized_screen/screen.hpp"
#include "../native_materialized_screen/data.hpp"
#include "../native_geometry_screen/screen.hpp"
#include "../native_constructor_generic_v2/constructor.hpp"
#include "../native_public_portfolio/internal.hpp"
#include "../../fast_routes.hpp"
#include "../../job_beam.hpp"
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
static bool distinct_seed_orders(const ds::DayProblem& problem) {
    std::array<int, kag::N_CROPS> plants{};
    for (const auto& work : problem.tile_work) for (const auto& action : work.actions)
        if (action.op == kag::OP_PLANT) ++plants[action.arg];
    for (int crop = 0; crop < kag::N_CROPS; ++crop) {
        if (problem.start.seeds[crop] >= plants[crop]) continue;
        if (problem.start.seeds[crop] > 0) return true;
        int first = 24;
        for (const auto& event : problem.market_plan)
            if (event.market_op == kag::M_BUY_SEED && event.item == crop && event.quantity > 0)
                first = std::min(first, int(event.hour));
        ds::InventoryCount supplied = 0;
        for (const auto& event : problem.market_plan)
            if (event.market_op == kag::M_BUY_SEED && event.item == crop && event.hour == first)
                supplied += event.quantity;
        if (supplied < plants[crop]) return true;
    }
    return false;
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
    auto keep = [&](exact::Result value, const std::string& stage, const ds::DayProblem* contract = nullptr) {
        if (!semantic::accepted(value)) return false;
        const auto replay = ds::replay_schedule(contract ? *contract : problem, *value.schedule);
        dc::require(replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied,
                    "quick final replay rejected schedule");
        if (options.accept_schedule && !options.accept_schedule(*value.schedule)) return false;
        result.winner = std::move(value); result.winning_stage = stage;
        return true;
    };
    std::array<bool, 5> tried_jobs{};
    day_scheduler::JobBeamStats whole_stats;
    // The last four variants change only seed assignment. When every required
    // seed of each crop has the same readiness, their routing and repair
    // models are identical to the first four.
    const bool seed_orders = distinct_seed_orders(problem);
    const int variants = seed_orders ? options.variants : std::min(options.variants, 4);
    // Spare-stock pickup repair is cheap on completed routes. Try a few more
    // route constructors before the stronger model on purchase-pressure days.
    const int jobs_after = options.job_problem ? std::min(
        materialized_screen::purchase_pressure(*options.job_problem) > 0 ? 4 : 1, variants - 1) : 0;
    auto try_jobs = [&](int kind = 0) {
        if (tried_jobs[kind] || result.accepted() || !options.fallback || remaining() <= 0 ||
            (kind && !options.job_problem)) return;
        tried_jobs[kind] = true;
        const bool fragment = kind == 1, resource_first = kind == 2, rebuild = kind == 4;
        const bool regret = kind == 3 || rebuild;
        const bool whole = kind == 0 || regret;
        const auto& contract = options.job_problem ? *options.job_problem : problem;
        day_scheduler::JobBeamOptions limits;
        limits.seconds = std::min(resource_first ? 1.0 : regret ? 2.0 : fragment ? 3.0 : 4.0, remaining());
        limits.completion_seconds = rebuild ? 0.4 : options.job_problem ? 2 : 1;
        limits.polish_seconds = options.job_problem ? 0.5 : 0.3;
        limits.completions = 2;
        limits.max_job_tasks = fragment ? 2 : 0;
        limits.terminal_capacity = options.job_problem != nullptr;
        limits.urgent_fragments = options.job_problem && whole;
        limits.prefer_unpolished = options.job_problem && whole;
        limits.raw_timed_hint = limits.prefer_unpolished;
        limits.seed_deadlines = limits.pickup_deadlines = options.job_problem && !resource_first;
        // The fragment retry follows whole-job search: try the purchase-aware
        // score first instead of spending its allowance on the same conflict.
        limits.alternate_purchase_scores = options.job_problem && whole;
        limits.regret_order = regret;
        if (regret) {
            limits.width = 1;
            limits.cache_routes = true;
            limits.lazy_rebuild = !rebuild;
            limits.rebuild_seconds = 0.5;
        }
        if (rebuild) {
            // The first constructors and fragment retry have failed. Spend
            // this remainder on several different complete route partitions.
            limits.first_variant = 2;
            limits.delivery_score = 1;
            limits.completions = 8;
        }
        if (resource_first) {
            limits.first_variant = 2; limits.variants = limits.completions = 1;
            limits.delivery_score = 1;
        }
        auto candidate = day_scheduler::job_beam(contract, limits, kind == 0 ? &whole_stats : nullptr);
        const std::string prefix = resource_first ? "job_resource/" : rebuild ? "job_rebuild/" : regret ? "job_regret/" : fragment ? "job_fragments/" : "jobs/";
        for (const auto& stage : candidate.stages)
            result.attempts.push_back({prefix + stage.name, portfolio::Attempt::Kind::constructor,
                sat::UNKNOWN, stage.seconds, stage.status, {}});
        if (candidate.schedule) {
            exact::Result value;
            value.solved = true; value.status = sat::FEASIBLE;
            value.schedule = std::move(candidate.schedule);
            value.replay = exact::ReplayCheck{true, true, true, true, {}};
            keep(std::move(value), "quick/" + prefix, &contract);
        }
    };
    std::array<bool, 4> proposed_modes{};
    std::array<bool, 2> tried_strong{};
    auto try_strong = [&](bool shared = false) {
        if (tried_strong[shared] || result.accepted() || !options.fallback) return;
        if (shared) {
            if (proposed_modes[0] || !proposed_modes[1] || remaining() < 3.0) return;
        } else if (!options.job_problem || !whole_stats.completed_partitions ||
            whole_stats.estimated_timing_feasible || remaining() < 4.0) return;
        tried_strong[shared] = true;
        // Select a stronger constructor using failures of the cheaper searches,
        // and retain time for its exact timing and inventory completion.
        const double before = now();
        const double stage_seconds = std::min(shared ? 8.0 : 4.0, remaining());
        const std::string stage = shared ? "early_shared" : "early_strong";
        dc::ConstructorOptions settings;
        settings.seconds = settings.repair.seconds = std::min(shared ? 4.0 : 3.0, stage_seconds - 1.0);
        if (shared) {
            settings.routing.shared_resource_flow = true;
            settings.repair.iterations = 30;
            settings.repair.shortlist = 16;
            settings.repair.plateau_steps = 12;
            settings.repair.optimize_balance = true;
        }
        auto routes = dc::construct(problem, settings);
        result.attempts.push_back({stage + "/construct", portfolio::Attempt::Kind::constructor,
            routes.proposal ? sat::FEASIBLE : sat::UNKNOWN, now() - before,
            "iterations=" + std::to_string(routes.iterations), {}});
        if (!routes.proposal) return;
        InternalHint hint;
        hint.type_workers = routes.proposal->type_workers;
        for (const auto& row : routes.proposal->assignments)
            hint.assignments.push_back({row.task, row.worker, row.rank, row.type, {}});
        auto backends = semantic::native_backends();
        backends.exact = [&](std::string_view name, const auto& p, const auto& h, const auto& limits) {
            const bool prepare = !exact_session;
            if (prepare) exact_session.emplace(p);
            auto budget = limits;
            if (shared && name == "direct_owner_exact") budget.seconds = std::min(budget.seconds, 1.0);
            auto value = exact_session->solve(h, budget, options.exact_tuning);
            if (prepare) value.build_seconds += exact_session->preparation_seconds();
            return value;
        };
        semantic::Session session(problem, std::move(backends));
        semantic::Options completion;
        completion.seconds = std::min(stage_seconds - (now() - before), remaining());
        if (completion.seconds <= 0) return;
        completion.workers = 1; completion.early_seconds = 2; completion.max_rounds = shared ? 8 : 6;
        auto value = session.repair(hint, completion);
        portfolio::Attempt attempt{stage + "/complete", portfolio::Attempt::Kind::semantic,
            value.accepted() ? sat::FEASIBLE : sat::UNKNOWN, value.seconds, {}, std::move(value.events)};
        result.attempts.push_back(std::move(attempt));
        if (value.accepted()) keep(std::move(*value.exact), stage + "/complete");
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
    for (int round = 0; round < options.rounds && !result.accepted(); ++round) {
    for (int variant = 0; variant < variants && !result.accepted(); ++variant) {
        if (remaining() <= 0) break;
        const int mode = variant % 4;
        dc::ConstructorOptions settings;
        const int effort = 1 << (2 * round);
        settings.iterations = options.iterations * effort;
        // Keep the small deterministic iteration schedules. The constructor
        // must stop at the day deadline; route_seconds bounds its repair step.
        settings.seconds = std::max(0.0, remaining());
        settings.repair.seconds = std::min(options.route_seconds, settings.seconds);
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
        if (routes.proposal) { event.status = sat::FEASIBLE; proposed_modes[mode] = true; }
        const bool routing_built = event.status != sat::MODEL_INVALID;
        event.seconds = now() - before;
        result.attempts.push_back(std::move(event));
        if (!routes.proposal) {
            if (options.fallback && routing_built && round == 0 && variant == 0 && remaining() > 0) {
                // The VRP feasibility flag includes approximate route limits.
                // Repair its best complete partition before escalating search.
                auto alternate = day_scheduler::fast_routes(problem, std::min(1.0, remaining()), 2);
                for (const auto& value : alternate.stages)
                    result.attempts.push_back({"partial/" + value.name, portfolio::Attempt::Kind::constructor,
                        sat::UNKNOWN, value.seconds, value.status, {}});
                if (alternate.schedule) {
                    exact::Result value;
                    value.solved = true;
                    value.status = sat::FEASIBLE;
                    value.schedule = std::move(alternate.schedule);
                    value.replay = exact::ReplayCheck{true, true, true, true, {}};
                    keep(std::move(value), "quick/partial_route_repair");
                }
            }
            if (round == 0 && variant == jobs_after) { try_jobs(); try_jobs(2); try_jobs(3); try_strong(); }
            continue;
        }
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
                    const double stock_seconds = s.seconds - constructed.total_seconds;
                    if (!constructed.schedule && stock_seconds > 0 &&
                        (constructed.materialize_rejection.starts_with("pickup stock not ready") ||
                         constructed.materialize_rejection.starts_with("deposit cargo not ready") ||
                         constructed.materialize_rejection == "availability not reached")) {
                        auto limits = s; limits.seconds = stock_seconds;
                        auto guarded = materialized_screen::solve_stock(p, h, limits, options.screen_tuning);
                        complete.semantic_events.push_back({"transfer_stock", semantic::Event::Kind::screen,
                            guarded.status, guarded.total_seconds, 0, guarded.materialize_rejection});
                        if (guarded.schedule) {
                            guarded.total_seconds += constructed.total_seconds;
                            constructed = std::move(guarded);
                        } else constructed.total_seconds += guarded.total_seconds;
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
        if (round == 0 && variant == jobs_after) { try_jobs(); try_jobs(2); try_jobs(3); try_strong(); }
        if (int(timing_retries.size()) >= options.geometry_after) try_timing();
    }
    if (round == 0) { try_jobs(1); try_jobs(4); }
    }
    try_jobs(1);
    try_jobs(4);
    if (!result.accepted() && options.variants > 0 && options.retry_seconds > 0 && remaining() > 0) {
        quick_v11::Options retry;
        retry.iterations = 64; retry.variants = seed_orders ? 8 : 4; retry.rounds = 1; retry.repair_steps = 12;
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
    // If only shared-resource quick construction found a route partition,
    // prioritize its stronger version before the ordinary native fallback.
    try_strong(true);
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
            // Use the rebuilt constructor so bundled portfolio orchestration
            // also respects its remaining time when it requests many iterations.
            backends.construct = [&](std::string_view, const auto& p, auto settings) {
                // A route proposal still needs timed completion. Spending the
                // entire remainder here can discard an otherwise useful route.
                const double left = std::max(0.0, remaining());
                const double completion_reserve = std::min(0.6, left * 0.5);
                settings.seconds = std::min(settings.seconds, left - completion_reserve);
                settings.repair.seconds = std::min(settings.repair.seconds, settings.seconds);
                return dc::construct(p, settings);
            };
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
