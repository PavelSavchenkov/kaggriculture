#pragma once
#include "repair.hpp"
#include "dump_data.hpp"

namespace day_constructor {
inline json::array plan_json(const BundlePlan& plan) {
    json::array result;
    for (const auto& route : plan) result.push_back(numbers(route));
    return result;
}
inline json::array group_json(const MoveGroup& group) {
    return {group.kind, group.source, group.target, pairs(group.deadlines)};
}
inline json::object repair_trace(const RepairStep& step, const NeighborhoodResult* neighborhood) {
    json::array visited;
    for (const auto& plan : step.visited) visited.push_back(plan_json(plan));
    json::object result{{"kind", neighborhood ? "select" : "finish"}, {"run", step.run}, {"step", neighborhood ? step.step : -1},
        {"plateau", step.plateau}, {"current", plan_json(step.current)}, {"best", plan_json(step.best)},
        {"current_score", numbers(step.current_score)}, {"best_score", numbers(step.best_score)},
        {"current_quality", numbers(step.current_quality)}, {"best_quality", numbers(step.best_quality)},
        {"visited", visited}, {"generated", step.generated}, {"evaluated", step.evaluated}};
    if (neighborhood) {
        json::array candidates;
        for (const auto& candidate : neighborhood->selected) candidates.push_back(json::object{
            {"cheap", numbers(candidate.cheap)}, {"group", group_json(candidate.group)}, {"detail", numbers(candidate.detail)}, {"plan", plan_json(candidate.plan)}});
        result["generated_now"] = neighborhood->generated;
        result["selected"] = candidates;
    }
    return result;
}
inline json::object repair_report(const RepairResult& result, const RepairOptions& options) {
    json::array deficits;
    for (const auto& row : result.seed_deficits) deficits.push_back(json::object{
        {"crop", row[0]}, {"hour", row[1]}, {"required", row[2]}, {"available", row[3]}, {"shortage", row[4]}});
    json::object details{{"precedence_lateness", result.precedence_lateness}, {"unmatched_routes", result.unmatched},
        {"fixed_point_impossible_routes", result.impossible}, {"unsupplied_inputs", result.unsupplied},
        {"seed_prefix_deficits", deficits}, {"seed_shortfall", result.seed_shortfall}, {"geometric_overflow", result.score[2]},
        {"delivery_collisions", result.delivery_collisions}, {"worker_assignment", numbers(result.workers)}};
    if (options.strong && options.iterations) {
        const auto& stats = result.stats;
        details["strong_generated_candidates"] = stats.generated;
        details["strong_evaluated_candidates"] = stats.evaluated;
        details["strong_visited_solutions"] = stats.visited;
        details["strong_best_violation"] = numbers(stats.best_score);
        details["strong_best_pickups"] = stats.best_pickups;
        details["strong_best_checkpoints"] = stats.best_checkpoints;
        details["strong_forced_splits"] = stats.forced_splits;
        std::vector<std::tuple<RepairScore, std::vector<int>, MoveGroup>> values;
        for (const auto& [group, summary] : stats.groups) values.emplace_back(summary.first, summary.second, group);
        std::sort(values.begin(), values.end());
        json::array groups;
        for (int index = 0; index < std::min(int(values.size()), 20); ++index) {
            const auto& [score, move, group] = values[index];
            groups.push_back(json::object{{"group", group_json(group)}, {"violation", numbers(score)}, {"move", numbers(move)}});
        }
        details["strong_best_groups"] = groups;
    }
    return {{"routes", plan_json(result.routes)}, {"lengths", numbers(result.lengths)}, {"score", numbers(result.score)}, {"details", details}};
}
}  // namespace day_constructor
