#pragma once
// Included inside day_native::exact, after the model's common types.

    // Prune only implications of a single hard restriction. Diagnostic
    // assumptions retain their original domains so releasing them is sound.
    std::vector<unsigned char> HintOptions::task_domains(int tasks, int workers) const {
        if (documents.size() != 1 || std::any_of(flags.begin(), flags.end(),
                [](auto flag) { return flag != HintFlag::fix_partial; })) return {};
        const auto& [kind, hint] = *documents.begin();
        const bool partial = kind == HintKind::fixed_partial ||
            (kind == HintKind::partial && flags.contains(HintFlag::fix_partial));
        const bool concrete = kind == HintKind::route_task;
        const bool grouped = kind == HintKind::route_type || kind == HintKind::route_owner;
        if (!partial && !concrete && !grouped) return {};
        std::vector<unsigned char> allowed(tasks * workers * hours, 1);
        auto restrict_task = [&](int task, const std::set<int>& members, int fixed_hour) {
            for (int worker = 0; worker < workers; ++worker)
                for (int hour = 0; hour < hours; ++hour)
                    allowed[(task * workers + worker) * hours + hour] =
                        members.contains(worker) && (fixed_hour < 0 || hour == fixed_hour);
        };
        std::set<int> seen;
        std::map<std::pair<int, int>, std::vector<int>> routes;
        for (const auto& assignment : hint.assignments) {
            const int task = assignment.task, worker = assignment.worker;
            require(task >= 0 && task < tasks && worker >= 0 && worker < workers, "hint task or worker out of range");
            if (!seen.insert(task).second) return {};
            if (partial) {
                const int hour = required(assignment.hour, "hour");
                require(hour >= 0 && hour < hours, "invalid partial assignment");
                if (kind != HintKind::fixed_partial || !free_tasks.contains(task)) restrict_task(task, {worker}, hour);
            } else {
                const int type = concrete ? 0 : required(assignment.type, "type");
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
                const auto& types = required(hint.type_workers, "type_workers");
                require(type >= 0 && type < int(types.size()), "invalid hinted worker type");
                for (int worker : types[type]) {
                    require(worker >= 0 && worker < workers, "invalid worker in type");
                    members.insert(worker);
                }
                require(members.contains(hinted_worker), "invalid hinted worker type");
            }
            for (int task : route_tasks) restrict_task(task, members, -1);
        }
        return allowed;
    }
