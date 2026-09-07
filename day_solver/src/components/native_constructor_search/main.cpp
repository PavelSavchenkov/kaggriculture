#include "trace.hpp"

int main(int argc, char** argv) {
    using namespace day_constructor;
    try {
        require(argc == 4, "usage: constructor_search_cli problem.json settings.json output_directory");
        const auto begin = std::chrono::steady_clock::now();
        const auto problem = ds::load_problem_json(argv[1]);
        std::ifstream option_file(argv[2]);
        require(bool(option_file), "cannot open settings");
        const auto settings = json::parse(std::string(std::istreambuf_iterator<char>(option_file), {}));
        Options options;
        SearchOptions search_options;
        bool trace = false;
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
            else if (key == "iterations") search_options.iterations = json::value_to<int>(value);
            else if (key == "restart_after") search_options.restart_after = json::value_to<int>(value);
            else if (key == "history_length") search_options.history_length = json::value_to<int>(value);
            else if (key == "exhaustive_on_best") search_options.exhaustive_on_best = value.as_bool();
            else if (key == "trace") trace = value.as_bool();
            else throw std::runtime_error("unsupported setting: " + std::string(key));
        }
        const RoutingData routing(problem, options);
        const auto data = routing.data();
        const fs::path output(argv[3]);
        require(fs::create_directories(output), "output already exists");
        Search search(data, options.seed);
        auto write = [&](const char* file, const json::value& value) {
            std::ofstream stream(output / file);
            stream << json::serialize(value) << '\n';
            require(bool(stream), "cannot write output");
        };
        if (trace) write("neighbours.json", neighbour_data(search));
        std::ofstream trace_file;
        SearchObserver observer;
        if (trace) {
            trace_file.open(output / "trace.jsonl");
            require(bool(trace_file), "cannot open trace");
            observer = [&](int iteration, const auto& current, const auto& candidate, const auto& best, const auto& evaluator, const auto& penalties, const auto& rng) {
                json::array values;
                for (double value : penalties.values) values.push_back(value);
                trace_file << json::serialize(json::object{
                    {"iteration", iteration}, {"current", solution_summary(current, evaluator)},
                    {"candidate", solution_summary(candidate, evaluator)}, {"best", solution_summary(best, evaluator)},
                    {"penalties", values}, {"rng", numbers(rng.state())}}) << '\n';
                require(bool(trace_file), "cannot write trace");
            };
        }
        const auto start_search = std::chrono::steady_clock::now();
        const auto best = search.run(search_options, observer);
        const double search_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_search).count();
        const vrp::CostEvaluator zero(std::vector<double>(data.numLoadDimensions(), 0), 0, 0);
        write("best.json", solution_routes(*best));
        auto report = solution_summary(*best, zero);
        report["iterations"] = search_options.iterations;
        report["rng"] = numbers(search.rng_state());
        report["search_calls"] = search.search_calls;
        report["restarts"] = search.restarts;
        report["best_updates"] = search.best_updates;
        report["accepted"] = search.accepted;
        report["history_updates"] = search.history_updates;
        report["search_seconds"] = search_seconds;
        report["total_seconds"] = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        report["scope"] = "routing proposal only; exact schedule completion still required";
        write("report.json", report);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
