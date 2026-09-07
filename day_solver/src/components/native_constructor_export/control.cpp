// Component control only. The supplied route is our own cold search output,
// never an original replay route. The public integration uses typed objects.
#include "report.hpp"
#include "../native_constructor_search/trace.hpp"

int main(int argc, char** argv) {
    using namespace day_constructor;
    try {
        require(argc == 5, "usage: export_control problem settings generated_routing_solution output");
        auto read = [](const fs::path& path) {
            std::ifstream file(path);
            require(bool(file), "cannot read component input");
            return json::parse(std::string(std::istreambuf_iterator<char>(file), {}));
        };
        const auto problem = ds::load_problem_json(argv[1]);
        const auto settings = read(argv[2]);
        Options options;
        bool split = true;
        double stage_remaining = 1000000;
        RepairOptions repair_options;
        repair_options.target_routes = problem.worker_count;
        for (const auto& [key, value] : settings.as_object()) {
            if (key == "fixed_cost") options.fixed_cost = json::value_to<int>(value);
            else if (key == "route_hours") options.route_hours = json::value_to<int>(value);
            else if (key == "seed") options.seed = json::value_to<int>(value);
            else if (key == "pair_jitter") options.pair_jitter = json::value_to<int>(value);
            else if (key == "include_all_tasks") options.include_all_tasks = value.as_bool();
            else if (key == "route_segments") options.route_segments = value.as_bool();
            else if (key == "worker_profile_fleet") options.worker_profile_fleet = value.as_bool();
            else if (key == "shared_resource_flow") options.shared_resource_flow = value.as_bool();
            else if (key == "separate_delivery_routes") options.separate_delivery_routes = value.as_bool();
            else if (key == "shared_seed_flow") options.shared_seed_flow = value.as_bool();
            else if (key == "split_resource_tasks") split = value.as_bool();
            else if (key == "stage_remaining_seconds") stage_remaining = json::value_to<double>(value);
            else if (key == "repair_iterations") repair_options.iterations = json::value_to<int>(value);
            else if (key == "target_routes") repair_options.target_routes = json::value_to<int>(value);
            else if (key == "shortlist") repair_options.shortlist = json::value_to<int>(value);
            else if (key == "plateau_steps") repair_options.plateau_steps = json::value_to<int>(value);
            else if (key == "strong") repair_options.strong = value.as_bool();
            else if (key == "optimize_balance") repair_options.optimize_balance = value.as_bool();
            else if (key == "optimize_pickups") repair_options.optimize_pickups = value.as_bool();
            else if (key == "optimize_checkpoints") repair_options.optimize_checkpoints = value.as_bool();
            else if (key == "separate_output_routes") repair_options.separate_output_routes = value.as_bool();
            else if (key == "repair_seconds") repair_options.seconds = json::value_to<double>(value);
            else if (key != "trace" && key != "iterations" && key != "restart_after" && key != "history_length" && key != "exhaustive_on_best")
                throw std::runtime_error("unsupported component setting: " + std::string(key));
        }
        const RoutingData routing(problem, options);
        const auto data = routing.data();
        const auto supplied = read(argv[3]);
        std::vector<vrp::Route> routes;
        for (const auto& route : supplied.as_array()[0].as_array()) {
            const auto& value = route.as_array();
            const auto& schedule = value[1].as_array();
            require(schedule.size() >= 2, "missing implicit routing depots");
            std::vector<vrp::Activity> activities;
            for (size_t i = 1; i + 1 < schedule.size(); ++i) {
                const auto& a = schedule[i].as_array();
                activities.emplace_back(vrp::Activity::ActivityType(json::value_to<int>(a[0])), json::value_to<size_t>(a[1]));
            }
            routes.emplace_back(data, activities, json::value_to<size_t>(value[0]));
        }
        const vrp::Solution solution(data, routes);
        require(solution_routes(solution) == supplied, "generated route reconstruction changed its schedule");
        const Bundles bundles(routing, solution, split);
        const fs::path output_dir(argv[4]);
        require(fs::create_directories(output_dir), "export output exists");
        const auto start = std::chrono::steady_clock::now();
        const auto result = repair_and_export(bundles, repair_options, [&] { return stage_remaining; });
        std::ofstream output(output_dir / "report.json"), hint(output_dir / "partial_route_type_hint.json");
        output << json::serialize(proposal_report(result, repair_options, problem.worker_count)) << '\n';
        hint << json::serialize(proposal_hint(result)) << '\n';
        require(bool(output) && bool(hint), "cannot write constructor export");
        std::ofstream timing(output_dir / "timing.json");
        timing << json::serialize(json::object{{"seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()}}) << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
