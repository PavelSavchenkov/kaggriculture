#pragma once

struct NativeHints {
    ExactModel& exact;
    const HintOptions& options;
    std::map<std::string, std::map<int, json::value>> cores;
    json::array restrictions;

    LinearExpr task_choices(int task, int worker) {
        require(task >= 0 && task < int(exact.tasks.size()) && worker >= 0 && worker < exact.workers,
                "hint task or worker out of range");
        LinearExpr choices;
        for (int hour = 0; hour < hours; ++hour) {
            const auto literal = exact.task_at[exact.task_index(task, worker, hour)];
            if (ExactModel::exists(literal)) choices += literal;
        }
        return choices;
    }

    void task_hint(int task, int worker, int hour) {
        if (hour < 0 || hour >= hours) return;
        const auto literal = exact.task_at[exact.task_index(task, worker, hour)];
        if (ExactModel::exists(literal)) exact.model.AddHint(literal, true);
    }

    BoolVar active(const std::string& label, bool diagnose, const std::string& core, const json::value& identity) {
        const auto literal = exact.boolean(label);
        if (diagnose) {
            exact.model.AddAssumption(literal);
            cores[core][literal.index()] = identity;
        } else exact.model.AddEquality(literal, 1);
        return literal;
    }

    void partial(const json::object& hint, bool fixed, bool diagnose, const std::set<int>& free) {
        std::set<std::tuple<int, int, int>> seen;
        for (const auto& value : hint.at("task_assignments").as_array()) {
            const auto& assignment = value.as_object();
            const int task = assignment.at("task").as_int64(), worker = assignment.at("worker").as_int64(),
                      hour = assignment.at("hour").as_int64();
            require(task >= 0 && task < int(exact.tasks.size()) && worker >= 0 && worker < exact.workers &&
                    hour >= 0 && hour < hours && seen.emplace(task, worker, hour).second, "invalid partial assignment");
            const auto literal = exact.task_at[exact.task_index(task, worker, hour)];
            require(ExactModel::exists(literal), "partial assignment outside exact task hour domain");
            if (!fixed || free.contains(task)) exact.model.AddHint(literal, true);
            else if (diagnose) {
                const auto assumption = active(name("fixed_partial", {task}), true, "fixed_partial_core", task);
                exact.model.AddEquality(literal, 1).OnlyEnforceIf(assumption);
            } else exact.model.AddEquality(literal, 1);
        }
    }

    void routes(const json::object& hint, const std::string& kind) {
        const bool concrete = kind == "task", owner = kind == "owner", chain = kind == "chain";
        const auto& assignments = hint.at("task_assignments").as_array();
        const bool partial = hint.if_contains("partial_routes") && hint.at("partial_routes").as_bool();
        require(assignments.size() == exact.tasks.size() || (!owner && partial), "route hint must cover tasks");
        std::vector<std::vector<int>> types;
        if (!concrete) {
            for (const auto& group : hint.at("type_workers").as_array()) {
                std::vector<int> members;
                for (const auto& member : group.as_array()) {
                    require(member.as_int64() >= 0 && member.as_int64() < exact.workers, "invalid worker in type");
                    members.push_back(member.as_int64());
                }
                require(std::set<int>(members.begin(), members.end()).size() == members.size(), "repeated worker in type");
                types.push_back(std::move(members));
            }
            if (owner) {
                std::vector<int> all, expected(exact.workers);
                for (const auto& members : types) all.insert(all.end(), members.begin(), members.end());
                std::sort(all.begin(), all.end());
                std::iota(expected.begin(), expected.end(), 0);
                require(all == expected, "owner route types must partition workers");
            }
        }
        std::map<std::tuple<int, int, int>, std::vector<std::pair<int, int>>> grouped;
        std::set<int> seen;
        for (const auto& value : assignments) {
            const auto& assignment = value.as_object();
            const int task = assignment.at("task").as_int64(), worker = assignment.at("worker").as_int64();
            require(task >= 0 && task < int(exact.tasks.size()) && seen.insert(task).second &&
                    worker >= 0 && worker < exact.workers, "invalid or repeated route task/worker");
            const int type = concrete ? 0 : int(assignment.at("type").as_int64());
            const int group = chain && assignment.if_contains("chain") ? int(assignment.at("chain").as_int64()) : worker;
            const int hour = owner ? 0 : int(assignment.at("hour").as_int64());
            grouped[{type, group, worker}].push_back({hour, task});
        }
        std::map<int, std::vector<BoolVar>> worker_routes;
        int route_index = -1;
        for (const auto& [key, supplied_values] : grouped) {
            ++route_index;
            const auto [type, group, hinted_worker] = key;
            if (options.free_routes.contains(concrete ? hinted_worker : route_index)) continue;
            if (!concrete) require(type >= 0 && type < int(types.size()) &&
                std::find(types[type].begin(), types[type].end(), hinted_worker) != types[type].end(), "invalid hinted worker type");
            const std::string prefix = chain ? "chain" : "route";
            const bool diagnose = options.flags.contains(concrete ? "--diagnose-route-task-core" :
                owner ? "--diagnose-task-owner-core" : "--diagnose-route-type-core") && !chain;
            const std::string core = concrete ? "fixed_route_task_core" : owner ? "fixed_route_owner_core" : "fixed_route_type_core";
            const auto enabled = active(name(concrete ? "fixed_concrete_route" : owner ? "fixed_owner_route" : "fixed_" + prefix,
                {concrete ? hinted_worker : route_index}), diagnose, core,
                concrete ? json::value(hinted_worker) : json::value(json::array{type, hinted_worker}));
            auto values = supplied_values;
            if (!owner) std::sort(values.begin(), values.end());
            if (concrete) {
                for (const auto& [hour, task] : supplied_values) {
                    auto choices = task_choices(task, hinted_worker);
                    require(!choices.variables().empty(), "worker has no legal task hours");
                    exact.model.AddEquality(choices, 1).OnlyEnforceIf(enabled);
                    task_hint(task, hinted_worker, hour);
                }
            } else {
                std::vector<BoolVar> candidates;
                for (int worker : types[type]) {
                    const auto selected = exact.boolean(name(owner ? "owner_route_worker" : prefix + "_worker", {route_index, worker}));
                    candidates.push_back(selected);
                    worker_routes[worker].push_back(selected);
                    exact.model.AddLessOrEqual(selected, enabled);
                    exact.model.AddHint(selected, worker == hinted_worker);
                }
                if (!owner) exact.model.AddEquality(LinearExpr::Sum(candidates), enabled);
                for (std::size_t index = 0; index < candidates.size(); ++index) {
                    const int worker = types[type][index];
                    for (const auto& [hour, task] : values) {
                        exact.model.AddEquality(task_choices(task, worker), candidates[index]).OnlyEnforceIf(enabled);
                        if (!owner && worker == hinted_worker) task_hint(task, worker, hour);
                    }
                }
                if (owner) exact.model.AddEquality(LinearExpr::Sum(candidates), enabled);
            }
            if (!owner)
                for (std::size_t index = 1; index < values.size(); ++index)
                    exact.model.AddLessThan(exact.events[values[index - 1].second], exact.events[values[index].second]).OnlyEnforceIf(enabled);
        }
        if (!concrete && !chain)
            for (const auto& [worker, choices] : worker_routes) exact.model.AddAtMostOne(choices);
    }

    void apply() {
        for (const std::string flag : {"--partial-hint", "--fixed-partial-hint", "--fixed-route-task-hint",
                                      "--fixed-route-type-hint", "--fixed-route-owner-type-hint", "--fixed-chain-type-hint"}) {
            if (!options.paths.contains(flag)) continue;
            const auto& hint = options.documents.at(flag);
            if (flag == "--partial-hint" || flag == "--fixed-partial-hint") {
                const bool fixed = flag == "--fixed-partial-hint" || options.flags.contains("--fix-partial-hint");
                partial(hint.as_object(), fixed, flag == "--fixed-partial-hint" && options.flags.contains("--diagnose-partial-core"),
                        flag == "--fixed-partial-hint" ? options.free_tasks : std::set<int>{});
                if (fixed) restrictions.emplace_back(flag == "--fixed-partial-hint" ? "fixed_partial_hint" : "partial_hint");
            } else {
                routes(hint.as_object(), flag == "--fixed-route-task-hint" ? "task" : flag == "--fixed-route-type-hint" ? "type" :
                       flag == "--fixed-route-owner-type-hint" ? "owner" : "chain");
                auto restriction = flag.substr(2);
                std::replace(restriction.begin(), restriction.end(), '-', '_');
                restrictions.emplace_back(restriction);
            }
        }
    }

    void report_cores(const sat::CpSolverResponse& response, json::object& report) const {
        if (response.status() != sat::INFEASIBLE) return;
        for (const auto& [name, identities] : cores) {
            json::array core;
            for (int literal : response.sufficient_assumptions_for_infeasibility())
                core.push_back(identities.contains(literal) ? identities.at(literal) : json::value(literal));
            report[name] = std::move(core);
        }
    }
};
