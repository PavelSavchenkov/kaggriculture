// Cold constructor: v3 input and algorithm settings only. No route input.
#include "constructor.hpp"
#include "report.hpp"

int main(int argc, char** argv) {
    using namespace day_constructor;
    try {
        require(argc == 4, "usage: constructor_cli problem settings output");
        const auto problem = ds::load_problem_json(argv[1]);
        std::ifstream input(argv[2]);
        require(bool(input), "cannot read constructor settings");
        const auto settings = json::parse(std::string(std::istreambuf_iterator<char>(input), {}));
        ConstructorOptions options;
        for (const auto& [key, value] : settings.as_object()) {
            auto integer = [&] { return json::value_to<int>(value); };
            if (key == "fixed_cost") options.routing.fixed_cost = integer();
            else if (key == "route_hours") options.routing.route_hours = integer();
            else if (key == "seed") options.routing.seed = integer();
            else if (key == "pair_jitter") options.routing.pair_jitter = integer();
            else if (key == "include_all_tasks") options.routing.include_all_tasks = value.as_bool();
            else if (key == "route_segments") options.routing.route_segments = value.as_bool();
            else if (key == "worker_profile_fleet") options.routing.worker_profile_fleet = value.as_bool();
            else if (key == "shared_resource_flow") options.routing.shared_resource_flow = value.as_bool();
            else if (key == "separate_delivery_routes") options.routing.separate_delivery_routes = value.as_bool();
            else if (key == "shared_seed_flow") options.routing.shared_seed_flow = value.as_bool();
            else if (key == "split_resource_tasks") options.split_resource_tasks = value.as_bool();
            else if (key == "use_all_workers") options.use_all_workers = value.as_bool();
            else if (key == "iterations") options.iterations = integer();
            else if (key == "time_limit") options.seconds = json::value_to<double>(value);
            else if (key == "repair_iterations") options.repair.iterations = integer();
            else if (key == "repair_shortlist") options.repair.shortlist = integer();
            else if (key == "repair_plateau_steps") options.repair.plateau_steps = integer();
            else if (key == "strong_repair") options.repair.strong = value.as_bool();
            else if (key == "optimize_balance") options.repair.optimize_balance = value.as_bool();
            else if (key == "optimize_pickups") options.repair.optimize_pickups = value.as_bool();
            else if (key == "optimize_checkpoints") options.repair.optimize_checkpoints = value.as_bool();
            else if (key == "repair_seconds") options.repair.seconds = json::value_to<double>(value);
            else throw std::runtime_error("unsupported constructor setting: " + std::string(key));
        }
        const fs::path output(argv[3]);
        require(fs::create_directories(output), "constructor output exists");
        const auto result = construct(problem, options);
        json::object report{{"status", result.proposal ? "INCUMBENT" : "UNKNOWN"},
            {"wall_time_seconds", result.seconds}, {"search_iterations", result.iterations}};
        if (result.proposal) {
            const auto& proposal = *result.proposal;
            const auto details = proposal_report(proposal, options.repair, problem.worker_count);
            for (const auto& [key, value] : details) report[key] = value;
            report["routes"] = proposal.repaired.routes.size(); report["tasks"] = proposal.assignments.size(); report["cost"] = result.cost;
            json::array route_stats;
            for (const auto& route : result.route_stats) route_stats.push_back(json::object{
                {"duration", route.duration}, {"distance", route.distance}, {"start_time", route.start}, {"end_time", route.end}, {"vehicle_type", route.vehicle}});
            report["route_stats"] = route_stats; report["resource_pairs"] = pairs(result.resource_pairs);
            auto mapping = [](const auto& values) {
                json::object object;
                for (auto [key, value] : values) object[std::to_string(key)] = value;
                return object;
            };
            report["late_purchase_tasks"] = mapping(result.late_inputs); report["late_seed_tasks"] = mapping(result.late_seeds);
            report["route_segments"] = options.routing.route_segments;
            report["worker_profile_fleet"] = options.routing.worker_profile_fleet;
            report["shared_resource_flow"] = options.routing.shared_resource_flow;
            report["shared_seed_flow"] = options.routing.shared_seed_flow;
            report["separate_delivery_routes"] = options.routing.separate_delivery_routes;
            report["delivery_variant"] = 0;
            report["optimize_checkpoints"] = options.repair.optimize_checkpoints;
            report["max_iterations"] = options.iterations;
            report["pair_jitter"] = options.routing.pair_jitter;
            report["seed"] = options.routing.seed;
            report["strong_repair"] = options.repair.strong;
            report["repair_shortlist"] = options.repair.shortlist;
            report["repair_plateau_steps"] = options.repair.plateau_steps;
            report["initial_hint"] = nullptr;
            report["hint"] = (output / "partial_route_type_hint.json").string();
            std::ofstream hint(output / "partial_route_type_hint.json");
            hint << json::serialize(proposal_hint(proposal)) << '\n';
            require(bool(hint), "cannot write constructor proposal");
        } else report["reason"] = "bounded resource-chain search found no feasible routes";
        std::ofstream file(output / "report.json");
        file << json::serialize(report) << '\n';
        require(bool(file), "cannot write constructor report");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
