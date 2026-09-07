#pragma once
#include "controller.hpp"
#include "problem_json.hpp"
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <cmath>

namespace semantic_adapter {
namespace json = boost::json;
namespace fs = std::filesystem;
namespace sat = operations_research::sat;
using namespace day_native;
constexpr int hours = day_solver::HOURS;
inline void require(bool condition, const std::string& reason) { if (!condition) throw std::runtime_error(reason); }
#include "../native_solver_api/json_output.hpp"

inline json::value read_json(const fs::path& path) {
    std::ifstream file(path);
    require(bool(file), "cannot read " + path.string());
    return json::parse(std::string(std::istreambuf_iterator<char>(file), {}));
}
inline int index(const json::value& value) {
    auto n = value.as_int64();
    require(n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max(), "index out of range");
    return int(n);
}
inline InternalHint read_hint(const json::value& value) {
    const auto& object = value.as_object();
    InternalHint hint;
    for (const auto& value : object.at("task_assignments").as_array()) {
        const auto& row = value.as_object();
        HintAssignment assignment{index(row.at("task")), index(row.at("worker")), {}, {}, {}};
        if (auto* v = row.if_contains("hour")) assignment.hour = index(*v);
        if (auto* v = row.if_contains("type")) assignment.type = index(*v);
        if (auto* v = row.if_contains("chain")) assignment.chain = index(*v);
        hint.assignments.push_back(assignment);
    }
    if (auto* groups = object.if_contains("type_workers")) hint.type_workers = json::value_to<std::vector<std::vector<int>>>(*groups);
    if (auto* partial = object.if_contains("partial_routes")) hint.partial_routes = partial->as_bool();
    return hint;
}
inline json::object hint_json(const InternalHint& hint) {
    json::array assignments;
    for (const auto& row : hint.assignments) {
        json::object value{{"task", row.task}, {"worker", row.worker}};
        if (row.hour) value["hour"] = *row.hour;
        if (row.type) value["type"] = *row.type;
        if (row.chain) value["chain"] = *row.chain;
        assignments.push_back(value);
    }
    json::object result{{"task_assignments", assignments}};
    if (hint.type_workers) result["type_workers"] = json::value_from(*hint.type_workers);
    if (hint.partial_routes) result["partial_routes"] = true;
    return result;
}
inline json::object exact_json(const exact::Result& result) {
    const std::string status = result.replay ? (result.replay->accepted ? "INCUMBENT" : "MODEL_REJECTED") : sat::CpSolverStatus_Name(result.status);
    json::object report{{"status", status}, {"solver_status", sat::CpSolverStatus_Name(result.status)},
        {"build_seconds", result.build_seconds}, {"wall_time_seconds", result.solver_seconds},
        {"branches", result.branches}, {"conflicts", result.conflicts},
        {"proof", {{"proven", false}, {"model_infeasible", result.solved && result.status == sat::INFEASIBLE},
                   {"restrictions", json::value_from(result.restrictions)}, {"relaxations", json::array{}}}}};
    if (result.replay) report["strict_replay"] = {{"accepted", result.replay->accepted}, {"strict", result.replay->strict},
        {"requirements", result.replay->requirements}, {"invariants", result.replay->invariants}, {"errors", json::value_from(result.replay->errors)}};
    return report;
}
inline json::object result_json(const semantic::Result& result) {
    static const char* kinds[] = {"screen", "exact", "cached_owner", "candidates"};
    json::array events;
    for (const auto& event : result.events) events.push_back({{"name", event.name}, {"kind", kinds[int(event.kind)]},
        {"solver_status", sat::CpSolverStatus_Name(event.status)}, {"seconds", event.seconds}, {"count", event.count},
        {"backend_error", event.backend_error}});
    json::object report{{"status", result.accepted() ? "SCHEDULE" : "UNKNOWN"}, {"events", events},
        {"deferred_owner_repair", result.deferred}, {"wall_time_seconds", result.seconds},
        {"winning_call", result.winning_call}, {"fixed_task_times", result.fixed_task_times},
        {"free_routes", json::value_from(result.free_routes)}, {"exact_report", nullptr}, {"winning_hint", nullptr}};
    if (result.exact) report["exact_report"] = exact_json(*result.exact);
    if (result.winning_hint) report["winning_hint"] = hint_json(*result.winning_hint);
    return report;
}
inline void set_option(semantic::Options& options, const std::string& flag, double value) {
    if (flag == "--workers" || flag == "--max-rounds" || flag == "--max-arcs")
        require(std::isfinite(value) && value == std::floor(value) && value >= 0 && value <= std::numeric_limits<int>::max(), "invalid integer option " + flag);
    if (flag == "--time-limit") options.seconds = value;
    else if (flag == "--early-completion-seconds") options.early_seconds = value;
    else if (flag == "--screen-seconds") options.screen_seconds = value;
    else if (flag == "--initial-screen-retry-seconds") options.initial_retry_seconds = value;
    else if (flag == "--hard-screen-seconds") options.hard_screen_seconds = value;
    else if (flag == "--exact-seconds") options.exact_seconds = value;
    else if (flag == "--screen-time-exact-seconds") options.task_time_seconds = value;
    else if (flag == "--screen-owner-seconds") options.screen_owner_seconds = value;
    else if (flag == "--late-owner-seconds") options.late_owner_seconds = value;
    else if (flag == "--workers") options.workers = int(value);
    else if (flag == "--max-rounds") options.max_rounds = int(value);
    else if (flag == "--max-arcs") options.max_arcs = int(value);
    else if (flag == "--fix-source-profiles") options.fix_source_profiles = bool(value);
    else if (flag == "--defer-infeasible-owner-repair") options.defer_owner_repair = bool(value);
    else throw std::runtime_error("unsupported option " + flag);
}
} // namespace semantic_adapter
