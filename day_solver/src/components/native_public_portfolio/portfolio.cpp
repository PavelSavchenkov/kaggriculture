#include "internal.hpp"
#include "problem_validation.hpp"
#include "replay.hpp"
#include <boost/multiprecision/cpp_int.hpp>

namespace day_native::portfolio {
namespace sat = operations_research::sat;
namespace ds = day_solver;
namespace dc = day_constructor;

bool Result::accepted() const { return winner && semantic::accepted(*winner); }

namespace detail {
using dc::require;
enum class Phase { unknown, schedule, infeasible };

bool needs_purchased_seeds(const ds::DayProblem& problem) {
    std::array<ds::InventoryCount, kag::N_CROPS> needed{};
    for (const auto& work : problem.tile_work) for (const auto& action : work.actions)
        if (action.op == kag::OP_PLANT) needed.at(action.arg) += action.quantity;
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        if (needed[crop] > problem.start.seeds[crop]) return true;
    return false;
}

static bool conserved_inventory(const ds::DayProblem& problem) {
    using Wide = boost::multiprecision::cpp_int;
    std::array<Wide, kag::N_ITEMS> shed;
    std::array<Wide, kag::N_CROPS> seeds;
    std::copy(problem.start.shed.begin(), problem.start.shed.end(), shed.begin());
    std::copy(problem.start.seeds.begin(), problem.start.seeds.end(), seeds.begin());
    for (const auto& event : problem.market_plan) {
        if (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL) shed.at(event.item) += event.quantity;
        else if (event.market_op == kag::M_BUY_SEED) seeds.at(event.item) += event.quantity;
    }
    for (const auto& work : problem.tile_work) for (const auto& action : work.actions) {
        if (action.output_item >= 0) shed.at(action.output_item) += action.output_quantity;
        if (action.op == kag::OP_FEED) shed[kag::WHEAT] -= action.quantity;
        else if (action.op == kag::OP_FERTILIZE) shed[kag::FERTILIZER] -= action.quantity;
        else if (action.op == kag::OP_PLACE) shed.at(action.arg) -= action.quantity;
        else if (action.op == kag::OP_PLANT) seeds.at(action.arg) -= action.quantity;
    }
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (shed[item] - problem.shed_availability.back()[item] != problem.end_shed[item]) return false;
    return std::equal(seeds.begin(), seeds.end(), problem.end_seeds.begin());
}

static InternalHint internal_hint(const dc::RouteProposal& proposal) {
    InternalHint result;
    result.type_workers = proposal.type_workers;
    result.partial_routes = true;
    for (const auto& value : proposal.assignments)
        result.assignments.push_back({value.task, value.worker, value.rank, value.type, {}});
    return result;
}

static bool unrestricted_infeasible(const exact::Result& result) {
    return result.solved && result.status == sat::INFEASIBLE && result.restrictions.empty();
}

struct Controller {
    const ds::DayProblem problem;
    Backends backends;
    semantic::Session session;
    Result result;

    Controller(const ds::DayProblem& supplied, Backends provided)
        : problem(supplied), backends(std::move(provided)), session(problem) {
        if (!backends.now) backends.now = semantic::native_backends().now;
        if (!backends.construct) backends.construct = [](std::string_view, const auto& p, auto o) { return dc::construct(p, o); };
        if (!backends.repair) backends.repair = [](std::string_view, const auto& p, auto o, const auto& hint) { return dc::repair_generated(p, o, hint); };
        if (!backends.semantic) backends.semantic = [this](std::string_view, const auto& hint, const auto& o) { return session.repair(hint, o); };
        if (!backends.screen) backends.screen = [](std::string_view, const auto& p, const auto& hint, const auto& o) { return screen::solve(p, hint, o); };
        if (!backends.exact) backends.exact = [](std::string_view, const auto& p, const auto& hint, const auto& o) { return exact::solve(p, hint, o); };
    }

    template<class Function> auto call(const std::string& stage, Attempt::Kind kind, Function&& function) {
        using Value = std::invoke_result_t<Function>;
        const double started = backends.now();
        Attempt event{stage, kind, sat::UNKNOWN, 0, {}, {}};
        Value value{};
        try {
            value = function();
            if constexpr (std::is_same_v<Value, dc::ConstructorResult>) event.status = value.proposal ? sat::FEASIBLE : sat::UNKNOWN;
            else if constexpr (std::is_same_v<Value, semantic::Result>) {
                event.status = value.accepted() ? sat::FEASIBLE : sat::UNKNOWN;
                event.semantic_events = value.events;
            } else event.status = value.status;
        } catch (const std::bad_alloc&) { throw; }
        catch (const std::exception& error) {
            // Existing process adapters report a rejected internal model and
            // preserve the remaining search. Public validation is outside here.
            event.status = sat::MODEL_INVALID;
            event.error = error.what();
        }
        event.seconds = backends.now() - started;
        result.attempts.push_back(std::move(event));
        return value;
    }

    bool keep(exact::Result value, const std::string& stage) {
        if (!semantic::accepted(value)) return false;
        const auto replay = ds::replay_schedule(problem, *value.schedule);
        require(replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied,
                "final strict replay rejected the proposed schedule");
        result.winner = std::move(value);
        result.winning_stage = stage;
        return true;
    }

    exact::Result exact_call(const std::string& label, double seconds, int workers, const exact::HintOptions& hints = {}) {
        exact::SolveOptions options;
        options.seconds = seconds; options.workers = workers;
        return call(label, Attempt::Kind::exact, [&] { return backends.exact(label, problem, hints, options); });
    }

    Phase resource(const ResourceOptions& options, const std::string& prefix) {
        const double started = backends.now();
        if (!conserved_inventory(problem)) {
            result.attempts.push_back({prefix + "/inventory_conservation", Attempt::Kind::conservation, sat::INFEASIBLE, 0, {}, {}});
            return Phase::infeasible;
        }
        std::size_t tasks = 0;
        for (const auto& work : problem.tile_work) tasks += work.actions.size();
        if (options.initial_exact_seconds > 0 && options.seconds > 0 && tasks <= 12 && problem.worker_count <= 4) {
            const auto label = prefix + "/initial_exact";
            auto exact = exact_call(label, std::min(options.initial_exact_seconds, options.seconds), options.workers);
            if (keep(exact, label)) return Phase::schedule;
            if (unrestricted_infeasible(exact)) return Phase::infeasible;
        }
        const bool seed_restart = !options.shared_seeds && needs_purchased_seeds(problem);
        double phase_limit = seed_restart ? std::min(options.initial_search_seconds, options.seconds) : options.seconds;
        auto remaining = [&] { return phase_limit - (backends.now() - started); };
        auto verification_remaining = [&] { return options.seconds - (backends.now() - started); };
        const double owner_seconds = std::max(options.shared_seeds ? 45.0 : 30.0, options.screen_owner_seconds);

        auto route_options = [&](double seconds) {
            dc::ConstructorOptions route;
            route.seconds = seconds;
            route.repair.seconds = std::min(30.0, seconds);
            route.routing.shared_seed_flow = options.shared_seeds;
            return route;
        };
        auto construct = [&](const std::string& stage, dc::ConstructorOptions settings) {
            const auto label = prefix + "/" + stage;
            return call(label, Attempt::Kind::constructor, [&] { return backends.construct(label, problem, settings); });
        };
        auto verify = [&](const dc::RouteProposal& proposal, const std::string& stage, double seconds,
                          double exact_seconds, int rounds, bool defer = false) {
            semantic::Options settings;
            settings.seconds = std::min(seconds, verification_remaining());
            settings.early_seconds = options.early_seconds;
            settings.exact_seconds = exact_seconds;
            settings.screen_owner_seconds = options.screen_owner_seconds;
            settings.workers = options.workers;
            settings.max_rounds = rounds; settings.max_arcs = 4;
            settings.defer_owner_repair = defer;
            const auto label = prefix + "/" + stage;
            const auto hint = internal_hint(proposal);
            auto value = call(label, Attempt::Kind::semantic, [&] { return backends.semantic(label, hint, settings); });
            if (value.accepted()) keep(std::move(*value.exact), label);
            return value.deferred;
        };

        std::optional<dc::RouteProposal> strong;
        bool deferred_strong = false;
        if (remaining() > 0) {
            auto value = construct("strong_resource_routes", route_options(std::min(options.strong_route_seconds, remaining())));
            strong = std::move(value.proposal);
            if (strong && verification_remaining() > 0)
                deferred_strong = verify(*strong, "semantic_route_repair", options.semantic_seconds, owner_seconds, 6,
                                         options.defer_strong && remaining() > 0);
        }
        if (result.accepted()) return Phase::schedule;
        if (remaining() > 0) {
            auto settings = route_options(std::min({35.0, options.strong_route_seconds, remaining()}));
            settings.repair.iterations = 30; settings.repair.shortlist = 16; settings.repair.plateau_steps = 12;
            settings.repair.optimize_balance = true; settings.routing.shared_resource_flow = true;
            auto value = construct("shared_cargo_routes", settings);
            if (value.proposal && verification_remaining() > 0)
                verify(*value.proposal, "shared_cargo_semantic_route_repair", options.semantic_seconds, owner_seconds, 8);
        }
        if (result.accepted()) return Phase::schedule;
        if (deferred_strong && verification_remaining() > 0)
            verify(*strong, "deferred_strong_route_repair", options.semantic_seconds, owner_seconds, 6);
        if (result.accepted()) return Phase::schedule;
        if (remaining() > 0) {
            auto settings = route_options(std::min(options.strong_route_seconds, remaining()));
            settings.repair.shortlist = 16; settings.repair.plateau_steps = 12; settings.repair.optimize_balance = true;
            auto value = construct("balanced_resource_routes", settings);
            if (value.proposal && verification_remaining() > 0)
                verify(*value.proposal, "balanced_semantic_route_repair", options.semantic_seconds, owner_seconds, 6);
        }
        if (result.accepted()) return Phase::schedule;
        if (remaining() > 0) {
            auto settings = route_options(std::min(20.0, remaining()));
            settings.routing.seed = 3; settings.routing.pair_jitter = 200;
            settings.routing.separate_delivery_routes = false;
            settings.repair.optimize_balance = true; settings.repair.seconds = settings.seconds;
            auto value = construct("diverse_resource_routes", settings);
            if (value.proposal && verification_remaining() > 0)
                verify(*value.proposal, "diverse_semantic_route_repair",
                       options.screen_owner_seconds > 0 ? std::max(30.0, options.semantic_seconds) : 30.0,
                       std::max(12.0, options.screen_owner_seconds), 6);
        }
        if (result.accepted()) return Phase::schedule;
        if (remaining() > 0 && strong) {
            auto settings = route_options(std::min(30.0, remaining()));
            settings.routing.seed = 0; settings.iterations = 0;
            settings.routing.separate_delivery_routes = false;
            settings.repair.optimize_pickups = settings.repair.optimize_balance = true;
            settings.repair.seconds = settings.seconds;
            const auto label = prefix + "/pickup_resource_routes";
            auto value = call(label, Attempt::Kind::repair, [&] { return backends.repair(label, problem, settings, strong->assignments); });
            if (value.proposal && verification_remaining() > 0)
                verify(*value.proposal, "pickup_semantic_route_repair", options.semantic_seconds, owner_seconds, 6);
        }
        if (result.accepted()) return Phase::schedule;
        for (int hours : options.route_hours) for (int seed : options.route_seeds) {
            if (remaining() <= 0) break;
            dc::ConstructorOptions settings;
            settings.routing = dc::Options{};
            settings.routing.include_all_tasks = true;
            settings.routing.route_hours = hours; settings.routing.seed = seed;
            settings.routing.shared_seed_flow = options.shared_seeds;
            settings.coalesce_patterns = settings.split_resource_tasks = settings.use_all_workers = false;
            settings.iterations = 0; settings.seconds = std::min(options.route_seconds, remaining());
            settings.repair.iterations = 100; settings.repair.plateau_steps = 4;
            settings.repair.strong = false; settings.repair.seconds = 0;
            const auto stage = "route_h" + std::to_string(hours) + "_s" + std::to_string(seed);
            auto value = construct(stage, settings);
            if (!value.proposal) continue;
            const auto hint = internal_hint(*value.proposal);
            for (int order_seed : options.order_seeds) {
                if (remaining() <= 0) break;
                const auto label = prefix + "/" + stage + "/order_s" + std::to_string(order_seed);
                screen::SolveOptions order;
                order.seconds = std::min(options.order_seconds, remaining());
                order.workers = options.order_workers; order.seed = order_seed;
                order.model.dynamic = options.dynamic_starts;
                const auto coarse = call(label, Attempt::Kind::screen, [&] { return backends.screen(label, problem, hint, order); });
                if (!coarse.solved || (coarse.status != sat::FEASIBLE && coarse.status != sat::OPTIMAL) || !coarse.hint || remaining() <= 0) continue;
                exact::HintOptions hints;
                hints.documents[HintKind::route_type] = *coarse.hint;
                auto exact = exact_call(label + "/exact", std::min(options.exact_seconds, remaining()), options.workers, hints);
                if (keep(std::move(exact), label + "/exact")) return Phase::schedule;
            }
        }
        phase_limit = options.seconds;
        if (seed_restart && remaining() > 0) {
            auto shared = options;
            shared.seconds = remaining(); shared.shared_seeds = true;
            shared.semantic_seconds = std::max(75.0, options.semantic_seconds);
            const auto outcome = resource(shared, prefix + "/shared_seed_restart");
            if (outcome != Phase::unknown) return outcome;
        }
        if (remaining() > 0 && options.cold_exact_seconds > 0) {
            const auto label = prefix + "/cold_exact";
            auto exact = exact_call(label, std::min(options.cold_exact_seconds, remaining()), options.workers);
            if (keep(exact, label)) return Phase::schedule;
            if (unrestricted_infeasible(exact)) return Phase::infeasible;
        }
        return Phase::unknown;
    }
};

static void budget(double value) { require(std::isfinite(value) && value >= 0, "budgets must be finite and nonnegative"); }

Result run(const ds::DayProblem& problem, const Options& options, Backends backends,
           const std::optional<ResourceOptions>& resource_only) {
    require(problem.format_version == 3, "public scheduler requires v3 input");
    const auto issues = ds::validate_problem(problem);
    require(issues.empty(), issues.empty() ? "" : issues.front().path + ": " + issues.front().message);
    budget(options.seconds); budget(options.fast_seconds); budget(options.early_seconds);
    require(options.workers > 0, "positive worker count required");
    Controller controller(problem, std::move(backends));
    const double started = controller.backends.now();
    if (resource_only) controller.resource(*resource_only, "resource");
    else for (const auto& phase : {"fast", "long_repair"}) {
        const double remaining = options.seconds - (controller.backends.now() - started);
        ResourceOptions resource;
        resource.seconds = std::string_view(phase) == "fast" ? std::min(options.fast_seconds, remaining) : remaining;
        if (resource.seconds <= 0) continue;
        resource.workers = options.workers; resource.early_seconds = options.early_seconds;
        if (std::string_view(phase) == "long_repair") {
            resource.semantic_seconds = 120; resource.screen_owner_seconds = 45;
            resource.shared_seeds = needs_purchased_seeds(problem);
        }
        controller.resource(resource, phase);
        if (controller.result.accepted()) break;
    }
    controller.result.seconds = controller.backends.now() - started;
    return std::move(controller.result);
}
} // namespace detail

Result solve(const ds::DayProblem& problem, const Options& options) { return detail::run(problem, options); }
} // namespace day_native::portfolio
