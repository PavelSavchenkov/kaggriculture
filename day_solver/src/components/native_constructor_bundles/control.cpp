// Component control only. The supplied route is our own cold search output,
// never an original replay route. The public integration uses typed objects.
#include "bundles.hpp"
#include "trace.hpp"

int main(int argc, char** argv) {
    using namespace day_constructor;
    try {
        require(argc == 5, "usage: bundles_control problem settings generated_routing_solution output");
        auto read = [](const fs::path& path) {
            std::ifstream file(path);
            require(bool(file), "cannot read component input");
            return json::parse(std::string(std::istreambuf_iterator<char>(file), {}));
        };
        const auto problem = ds::load_problem_json(argv[1]);
        const auto settings = read(argv[2]);
        Options options;
        bool split = true;
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
        json::array assignments, members, task_bundle, ordered_routes, predecessors, workers, supply;
        for (const auto& assignment : bundles.assignments) assignments.push_back(json::array{assignment.task, assignment.route, assignment.rank});
        for (const auto& [key, ids] : bundles.members) members.push_back(json::array{numbers(std::array{key.first, key.second}), numbers(ids)});
        for (Key key : bundles.task_bundle) task_bundle.push_back(json::array{key.first, key.second});
        for (const auto& route : bundles.routes) ordered_routes.push_back(pairs(route));
        for (const auto& [key, values] : bundles.predecessors) predecessors.push_back(json::array{json::array{key.first, key.second}, pairs(values)});
        for (const auto& worker : bundles.workers) workers.push_back(json::array{numbers(worker.start), worker.release});
        for (const auto& [crop, curve] : bundles.seed_supply) supply.push_back(json::array{crop, numbers(curve)});
        json::array free;
        for (int item : routing.input_items) free.push_back(json::array{item, routing.free_inputs[item]});
        const json::object result{{"assignments", assignments}, {"members", members}, {"task_bundle", task_bundle},
            {"routes", ordered_routes}, {"predecessors", predecessors}, {"early", numbers(bundles.early)}, {"late", numbers(bundles.late)},
            {"capacities", numbers(bundles.capacities)}, {"workers", workers}, {"seed_supply", supply},
            {"input_ready", pairs(routing.input_ready)}, {"free_inputs", free}};
        std::ofstream output(argv[4]);
        output << json::serialize(result) << '\n';
        require(bool(output), "cannot write bundle output");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
