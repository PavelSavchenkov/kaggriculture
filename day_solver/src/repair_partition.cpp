#include "repair_partition.hpp"
#include "components/native_constructor_generic_v2/constructor.hpp"
#include "components/native_unnamed_exact/exact.hpp"
#include "replay.hpp"

namespace day_scheduler {
Result repair_partition(const day_solver::DayProblem& problem, const day_native::InternalHint& proposal,
                        double seconds, double attempt_seconds, int candidates) {
    namespace dc = day_constructor;
    namespace dn = day_native;
    namespace sat = operations_research::sat;
    dc::require(std::isfinite(seconds) && seconds >= 0 && std::isfinite(attempt_seconds) &&
        attempt_seconds > 0 && candidates > 0, "invalid partition repair budget");
    const auto started = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count(); };
    Result result;
    auto finish = [&] { result.seconds = elapsed(); return std::move(result); };
    if (!seconds) return finish();
    const dc::TaskData data(problem);
    const auto& tasks = data.tasks;
    dc::require(proposal.assignments.size() == tasks.size(), "repair requires a complete internal partition");
    std::vector<int> owners(tasks.size(), -1);
    std::map<int, std::vector<std::pair<int, int>>> routes;
    for (const auto& task : proposal.assignments) {
        dc::require(task.task >= 0 && task.task < int(tasks.size()) && owners[task.task] < 0 &&
            task.worker >= 0 && task.worker < problem.worker_count, "invalid repair task or owner");
        owners[task.task] = task.worker;
        routes[task.worker].emplace_back(dn::required(task.hour, "rank"), task.task);
    }
    std::map<int, int> duration, lateness;
    std::vector<int> ids;
    for (auto& [worker, route] : routes) {
        ids.push_back(worker);
        std::sort(route.begin(), route.end());
        auto point = dc::shed.front();
        int time = worker ? data.hires.at(worker - 1) + 1 : 0;
        if (worker) point = *std::min_element(dc::shed.begin(), dc::shed.end(), [&](auto a, auto b) {
            return dc::distance(a, tasks[route.front().second].point) < dc::distance(b, tasks[route.front().second].point);
        });
        for (auto [rank, id] : route) {
            time = std::max(time + dc::distance(point, tasks[id].point), data.early[id]);
            lateness[worker] += std::max(0, time - data.late[id]);
            ++time; point = tasks[id].point;
        }
        duration[worker] = time;
    }
    using Choice = std::tuple<int, int, int, int, int, int>;
    std::vector<Choice> ranked;
    for (int a : ids) for (int b : ids) if (a < b) {
        int overlap = 0, closest = 20;
        for (auto [rank_a, task_a] : routes.at(a)) for (auto [rank_b, task_b] : routes.at(b)) {
            const int distance = dc::distance(tasks[task_a].point, tasks[task_b].point);
            closest = std::min(closest, distance);
            overlap += distance == 0;
        }
        ranked.emplace_back(-lateness[a] - lateness[b], -overlap, closest, -duration[a] - duration[b], a, b);
    }
    std::sort(ranked.begin(), ranked.end());
    std::vector<std::set<int>> neighborhoods;
    std::set<std::set<int>> seen;
    for (const auto& row : ranked) {
        const auto [late, overlap, distance, length, a, b] = row;
        std::set<int> selected{a, b};
        if (seen.insert(selected).second) neighborhoods.push_back(std::move(selected));
        if (int(neighborhoods.size()) >= candidates) break;
    }
    // Also try a larger neighborhood around the best pair, without deleting
    // the pair controls. Ownership and order of every removed task are free.
    if (ids.size() > 2 && !neighborhoods.empty()) {
        const auto base = neighborhoods.front();
        for (const auto& row : ranked) {
            auto selected = base;
            selected.insert(std::get<4>(row)); selected.insert(std::get<5>(row));
            if (selected.size() == 3 && seen.insert(selected).second) neighborhoods.push_back(std::move(selected));
            if (int(neighborhoods.size()) >= candidates + 2) break;
        }
    }
    dn::unnamed::Session exact(problem);
    result.stages.push_back({"atomic/prepare", "BUILT", elapsed()});
    for (const auto& selected : neighborhoods) {
        if (elapsed() >= seconds) break;
        auto hint = proposal;
        hint.partial_routes = true;
        std::erase_if(hint.assignments, [&](const auto& row) { return selected.contains(row.worker); });
        dn::exact::HintOptions hints;
        hints.documents[dn::HintKind::route_task] = std::move(hint);
        dn::exact::SolveOptions limits;
        limits.seconds = std::min(attempt_seconds, seconds - elapsed()); limits.workers = 1;
        std::string name = "atomic/free";
        for (int worker : selected) name += "_" + std::to_string(worker);
        auto completed = exact.solve(hints, limits, 2);
        result.stages.push_back({name + "/build", "BUILT", completed.build_seconds});
        result.stages.push_back({name + "/search", sat::CpSolverStatus_Name(completed.status), completed.solver_seconds});
        result.stages.push_back({name + "/variables", std::to_string(completed.variables), 0});
        if (!completed.schedule) continue;
        const auto replay = day_solver::replay_schedule(problem, *completed.schedule);
        dc::require(replay.candidate.replay.strict_valid && replay.requirements_satisfied &&
            replay.invariants_satisfied && replay.errors.empty(), "partition repair failed strict replay");
        result.schedule = std::move(completed.schedule);
        break;
    }
    return finish();
}
}
