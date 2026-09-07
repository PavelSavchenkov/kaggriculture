#pragma once

// Internal candidates come from our cold route constructor. The public pipeline
// still accepts v3 input only; original replay routes must never enter this API.
struct HintOptions {
    std::map<std::string, fs::path> paths;
    std::map<std::string, json::value> documents;
    std::set<std::string> flags;
    std::set<int> free_routes, free_tasks;

    bool parse(const std::string& option, int& index, int argc, char** argv) {
        static const std::set<std::string> path_flags{
            "--partial-hint", "--fixed-partial-hint", "--fixed-route-task-hint",
            "--fixed-route-type-hint", "--fixed-route-owner-type-hint", "--fixed-chain-type-hint"};
        static const std::set<std::string> boolean_flags{
            "--fix-partial-hint", "--diagnose-partial-core", "--diagnose-route-task-core",
            "--diagnose-route-type-core", "--diagnose-task-owner-core"};
        if (boolean_flags.contains(option)) { flags.insert(option); return true; }
        if (!path_flags.contains(option) && option != "--free-fixed-route" && option != "--free-partial-task") return false;
        require(index + 1 < argc, "missing hint option value");
        const std::string value = argv[++index];
        if (path_flags.contains(option)) {
            require(!paths.contains(option), "duplicate hint option");
            paths[option] = value;
        } else {
            const int number = std::stoi(value);
            require(number >= 0, "negative free task/route");
            (option == "--free-fixed-route" ? free_routes : free_tasks).insert(number);
        }
        return true;
    }

    void load() {
        for (const auto& [flag, path] : paths) {
            std::ifstream input(path);
            require(bool(input), "cannot read internal hint");
            documents[flag] = json::parse(std::string(std::istreambuf_iterator<char>(input), {}));
        }
    }

    // These are implications of a single hard restriction, not new constraints.
    // Assumptions must retain their original domains so releasing them is sound.
    std::vector<unsigned char> task_domains(int tasks, int workers) const {
        if (documents.size() != 1 || std::any_of(flags.begin(), flags.end(),
                [](const auto& flag) { return flag.starts_with("--diagnose-"); })) return {};
        const auto& [flag, document] = *documents.begin();
        const bool partial = flag == "--fixed-partial-hint" ||
            (flag == "--partial-hint" && flags.contains("--fix-partial-hint"));
        const bool concrete = flag == "--fixed-route-task-hint";
        const bool grouped = flag == "--fixed-route-type-hint" || flag == "--fixed-route-owner-type-hint";
        if (!partial && !concrete && !grouped) return {};
        const auto& hint = document.as_object();
        std::vector<unsigned char> allowed(tasks * workers * hours, 1);
        auto restrict_task = [&](int task, const std::set<int>& members, int fixed_hour) {
            for (int worker = 0; worker < workers; ++worker)
                for (int hour = 0; hour < hours; ++hour)
                    allowed[(task * workers + worker) * hours + hour] =
                        members.contains(worker) && (fixed_hour < 0 || hour == fixed_hour);
        };
        std::set<int> seen;
        std::map<std::pair<int, int>, std::vector<int>> routes;
        for (const auto& value : hint.at("task_assignments").as_array()) {
            const auto& assignment = value.as_object();
            const auto task = assignment.at("task").as_int64(), worker = assignment.at("worker").as_int64();
            require(task >= 0 && task < tasks && worker >= 0 && worker < workers, "hint task or worker out of range");
            // Repeated partial assignments can be contradictory; keep their
            // literals so the original model and hint validation decide that.
            if (!seen.insert(task).second) return {};
            if (partial) {
                const auto hour = assignment.at("hour").as_int64();
                require(hour >= 0 && hour < hours, "invalid partial assignment");
                if (flag != "--fixed-partial-hint" || !free_tasks.contains(task))
                    restrict_task(task, {int(worker)}, hour);
            } else {
                const int type = concrete ? 0 : int(assignment.at("type").as_int64());
                routes[{type, worker}].push_back(task);
            }
        }
        int route_index = -1;
        for (const auto& [key, route_tasks] : routes) {
            const auto [type, hinted_worker] = key;
            ++route_index;
            if (free_routes.contains(concrete ? hinted_worker : route_index)) continue;
            std::set<int> members;
            if (concrete) members.insert(hinted_worker);
            else {
                const auto& types = hint.at("type_workers").as_array();
                require(type >= 0 && type < int(types.size()), "invalid hinted worker type");
                for (const auto& member : types[type].as_array()) {
                    const auto worker = member.as_int64();
                    require(worker >= 0 && worker < workers, "invalid worker in type");
                    members.insert(worker);
                }
                require(members.contains(hinted_worker), "invalid hinted worker type");
            }
            for (int task : route_tasks) restrict_task(task, members, -1);
        }
        return allowed;
    }
};
