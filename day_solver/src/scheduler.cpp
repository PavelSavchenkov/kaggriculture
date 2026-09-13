#include "day_solver/scheduler.hpp"
#include "components/native_quick_portfolio_v30/quick.hpp"
#include "storage.hpp"
#include "fixed_path_stock.hpp"
#include "schedule_hint.hpp"
#include "fast_routes.hpp"
#include "job_beam.hpp"
#include "replay.hpp"
#include "problem_validation.hpp"
#include <chrono>

namespace day_scheduler {
Result solve(const day_solver::DayProblem& problem, const Options& options) {
    namespace ds = day_solver;
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    std::vector<Stage> stages;
    auto finish = [&](std::optional<std::array<kag::Action, ds::HOURS>> schedule) {
        return Result{std::move(schedule), elapsed(), std::move(stages)};
    };
    auto record_stages = [&](const auto& value, const std::string& prefix) {
        for (const auto& attempt : value.attempts) {
            stages.push_back({prefix + attempt.stage,
                operations_research::sat::CpSolverStatus_Name(attempt.status), attempt.seconds});
            if (!attempt.error.empty()) stages.push_back({prefix + attempt.stage + "/detail", attempt.error, 0});
            for (const auto& event : attempt.semantic_events) {
                const auto name = prefix + attempt.stage + "/" + event.name;
                stages.push_back({name, operations_research::sat::CpSolverStatus_Name(event.status), event.seconds});
                if (!event.backend_error.empty()) stages.push_back({name + "/detail", event.backend_error, 0});
            }
        }
    };
    const auto issues = ds::validate_problem(problem);
    if (!issues.empty()) throw std::runtime_error(issues.front().path + ": " + issues.front().message);
    if (!std::isfinite(options.seconds) || options.seconds < 0 || options.fallback_workers <= 0)
        throw std::runtime_error("invalid solve budget or fallback worker count");
    if (options.search != Search::Portfolio && options.search != Search::Regret &&
        options.search != Search::RegretDeferred && options.search != Search::RegretFast)
        throw std::runtime_error("invalid search policy");
    if (options.seconds == 0) return {};
    if (options.search != Search::Portfolio) {
        JobBeamOptions limits;
        limits.seconds = options.seconds;
        limits.completion_seconds = 2;
        limits.polish_seconds = 0.5;
        limits.width = 1;
        limits.regret_order = true;
        limits.cache_routes = true;
        limits.defer_polish = options.search != Search::Regret;
        if (options.search == Search::RegretFast) limits.raw_completion_share = 0.9;
        limits.terminal_capacity = problem.start.shed_capacity != storage::unlimited;
        limits.urgent_fragments = limits.prefer_unpolished = limits.raw_timed_hint = true;
        limits.seed_deadlines = limits.pickup_deadlines = limits.alternate_purchase_scores = true;
        auto result = job_beam(problem, limits);
        result.seconds = elapsed();
        return result;
    }
    // Reserve an early attempt for the cheap route family. Previously the
    // legacy constructors could consume the whole budget before reaching it.
    // Keep the established order for longer calls: this reservation regressed
    // a 12-second cold control despite improving the four-second cohorts.
    if (options.seconds >= 1.0 && options.seconds <= 4.0) {
        auto fast = options;
        fast.search = Search::RegretFast;
        fast.seconds = std::min(2.0, options.seconds * 0.5);
        auto candidate = solve(problem, fast);
        for (auto stage : candidate.stages) {
            stage.name = "fast_front/" + stage.name;
            stages.push_back(std::move(stage));
        }
        if (candidate.schedule) return finish(std::move(candidate.schedule));
        // An extra worker changes greedy assignments. Retry a smaller route
        // set, then restore the final hire as an idle worker in its exact slot.
        // Removing the last chronological hire cannot move an earlier worker.
        if (problem.worker_count > 1 && options.seconds - elapsed() > 0.05) {
            auto smaller = problem;
            auto last = smaller.market_plan.end();
            for (auto it = smaller.market_plan.begin(); it != smaller.market_plan.end(); ++it)
                if (it->market_op == kag::M_HIRE && (last == smaller.market_plan.end() ||
                    std::pair(it->hour, it->order_index) > std::pair(last->hour, last->order_index))) last = it;
            if (last == smaller.market_plan.end()) throw std::logic_error("missing last hire");
            const int removed_hour = last->hour;
            smaller.market_plan.erase(last);
            --smaller.worker_count;
            prepare_problem(smaller);
            fast.seconds = std::min(1.5, (options.seconds - elapsed()) * 0.75);
            auto alternate = solve(smaller, fast);
            for (auto stage : alternate.stages) {
                stage.name = "smaller_routes/" + stage.name;
                stages.push_back(std::move(stage));
            }
            if (alternate.schedule) {
                auto& schedule = *alternate.schedule;
                for (int hour = 0; hour < ds::HOURS; ++hour) {
                    auto& action = schedule[hour];
                    if (hour > removed_hour) {
                        if (action.n_units != smaller.worker_count) throw std::logic_error("last hire worker mismatch");
                        action.units[action.n_units++] = {};
                    }
                    action.n_orders = 0;
                    std::fill(std::begin(action.orders), std::end(action.orders), kag::Order{});
                }
                for (const auto& event : problem.market_plan) {
                    auto& action = schedule[event.hour];
                    action.n_orders = std::max(action.n_orders, int(event.order_index) + 1);
                    action.orders[event.order_index] = {event.market_op, uint8_t(std::max(0, int(event.item))), event.quantity};
                }
                for (auto& action : schedule) action.finalize();
                const auto replay = ds::replay_schedule(problem, schedule);
                if (!replay.requirements_satisfied || !replay.invariants_satisfied ||
                    !replay.candidate.replay.strict_valid || !replay.errors.empty())
                    throw std::logic_error("smaller route schedule does not satisfy original contract");
                return finish(std::move(alternate.schedule));
            }
        }
    }
    if (elapsed() >= options.seconds) return finish({});
    day_native::quick_v30::Options settings;
    settings.iterations = 64;
    settings.variants = 8;
    settings.rounds = 2;
    settings.fallback = true;
    settings.reference.seconds = std::max(0.0, options.seconds - elapsed());
    settings.reference.workers = options.fallback_workers;
    if (problem.start.shed_capacity == storage::unlimited) {
        auto result = day_native::quick_v30::solve(problem, settings);
        record_stages(result, "");
        if (result.accepted()) return finish(std::move(result.winner->schedule));
        return finish({});
    }
    // The routing portfolio proposes a schedule without storage losses. Keep
    // the real endpoint for acceptance and account for possible overflow only
    // in this proposal's conservation equations.
    auto relaxed = problem;
    relaxed.start.shed_capacity = storage::unlimited;
    auto balance = problem.start.shed;
    for (const auto& event : problem.market_plan)
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)
            balance[event.item] += event.quantity;
    for (const auto& work : problem.tile_work) for (const auto& action : work.actions) {
        if (action.output_item >= 0) balance[action.output_item] += action.output_quantity;
        if (action.op == kag::OP_FEED) balance[kag::WHEAT] -= action.quantity;
        if (action.op == kag::OP_FERTILIZE) balance[kag::FERTILIZER] -= action.quantity;
        if (action.op == kag::OP_PLACE) balance[action.arg] -= action.quantity;
    }
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        balance[item] -= problem.shed_availability.back()[item];
        if (balance[item] < problem.end_shed[item] || balance[item] > ds::MAX_INPUT_COUNT)
            return finish({});
    }
    auto set_endpoint = [&](auto& proposal, const auto& endpoint) {
        proposal.end_shed = endpoint;
        for (auto& bound : proposal.required_outcomes)
            if (bound.key.metric == ds::OutcomeMetric::END_SHED)
                bound.lower = bound.upper = endpoint[bound.key.subject];
    };
    set_endpoint(relaxed, balance);
    std::optional<std::array<kag::Action, ds::HOURS>> bounded, proposal_hint;
    double stock_repair_seconds = 0;
    settings.job_problem = &problem;
    settings.accept_schedule = [&](const auto& schedule) {
        const auto before = elapsed();
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(std::clamp(options.seconds - elapsed(), 0.0, 0.05)));
        bounded = storage::repair(problem, schedule, deadline);
        if (!bounded) {
            const double allowance = std::min(0.2 - stock_repair_seconds, options.seconds - elapsed());
            if (allowance > 0) {
                const auto hint = schedule_hint(relaxed, schedule);
                const auto completed = fixed_path_stock::solve(problem, schedule, hint, allowance, true);
                stock_repair_seconds += completed.seconds;
                for (auto stage : completed.stages) {
                    stage.name = "capacity/" + stage.name;
                    stages.push_back(std::move(stage));
                }
                bounded = completed.schedule;
            }
        }
        stages.push_back({"capacity/candidate", bounded ? "SCHEDULE" : "REJECTED", elapsed() - before});
        if (!bounded) proposal_hint = schedule;
        return bool(bounded);
    };
    auto result = day_native::quick_v30::solve(relaxed, settings);
    record_stages(result, "proposal/");
    if (bounded) return finish(std::move(bounded));
    // A different route partition can change deposit timing and night cargo
    // order without imposing extra deadlines on every ordinary day.
    if (proposal_hint && elapsed() < options.seconds) {
        auto alternate = fast_routes(relaxed, std::min(1.0, options.seconds - elapsed()), 2);
        for (auto& stage : alternate.stages) {
            stage.name = "capacity/alternate/" + stage.name;
            stages.push_back(std::move(stage));
        }
        if (alternate.schedule) {
            const auto deadline = std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(std::max(0.0, std::min(0.05, options.seconds - elapsed()))));
            const auto before = elapsed();
            auto repaired = storage::repair(problem, *alternate.schedule, deadline);
            stages.push_back({"capacity/alternate/validate", repaired ? "SCHEDULE" : "UNKNOWN", elapsed() - before});
            if (repaired) return finish(std::move(repaired));
        }
    }
    // Complete our own proposed routes with explicit capacity constraints.
    // No replay routes or arbitrary blanket delivery deadlines enter search.
    day_native::InternalHint hint;
    if (proposal_hint) hint = schedule_hint(relaxed, *proposal_hint);
    for (int attempt = 0; attempt < 2 && elapsed() < options.seconds; ++attempt) {
        day_native::exact::HintOptions hints;
        if (!hint.assignments.empty())
            hints.documents[attempt == 0 ? day_native::HintKind::fixed_partial : day_native::HintKind::route_type] = hint;
        day_native::exact::SolveOptions limits;
        limits.seconds = std::min(options.seconds - elapsed(), attempt == 0 ? 0.5 : options.seconds);
        limits.workers = options.fallback_workers;
        const auto completed = day_native::exact::solve(problem, hints, limits);
        const std::string stage = attempt == 0 ? "capacity/fixed_work" : "capacity/route_assignment";
        stages.push_back({stage + "/build", "BUILT", completed.build_seconds});
        stages.push_back({stage + "/search", operations_research::sat::CpSolverStatus_Name(completed.status), completed.solver_seconds});
        if (completed.replay && completed.replay->accepted) return finish(std::move(completed.schedule));
        if (hint.assignments.empty()) break;
    }
    return finish({});
}
}
