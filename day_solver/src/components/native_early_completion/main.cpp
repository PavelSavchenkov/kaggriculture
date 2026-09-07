#include "completion.hpp"
#include "problem_json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <boost/json.hpp>

namespace json = boost::json;
namespace fs = std::filesystem;
namespace sat = operations_research::sat;
using namespace day_native;
constexpr int hours = day_solver::HOURS;
void require(bool value, const std::string& reason) {
    if (!value) throw std::runtime_error(reason);
}
#include "../native_solver_api/json_output.hpp"

static int index(const json::value& value) {
    const auto number = value.as_int64();
    require(number >= std::numeric_limits<int>::min() && number <= std::numeric_limits<int>::max(), "hint index out of range");
    return int(number);
}
static InternalHint read_hint(const fs::path& path) {
    std::ifstream file(path);
    require(bool(file), "cannot read internal hint");
    const auto value = json::parse(std::string(std::istreambuf_iterator<char>(file), {}));
    const auto& object = value.as_object();
    InternalHint hint;
    for (const auto& value : object.at("task_assignments").as_array()) {
        const auto& row = value.as_object();
        hint.assignments.push_back({index(row.at("task")), index(row.at("worker")), index(row.at("hour")), {}, {}});
    }
    if (const auto* groups = object.if_contains("type_workers")) {
        hint.type_workers.emplace();
        for (const auto& group : groups->as_array()) {
            std::vector<int> workers;
            for (const auto& worker : group.as_array()) workers.push_back(index(worker));
            hint.type_workers->push_back(std::move(workers));
        }
    }
    return hint;
}
static json::object hint_json(const InternalHint& hint) {
    json::array assignments;
    for (const auto& row : hint.assignments) assignments.push_back({{"task", row.task}, {"worker", row.worker},
        {"hour", required(row.hour, "hour")}, {"type", required(row.type, "type")}});
    return {{"task_assignments", assignments}, {"type_workers", json::value_from(required(hint.type_workers, "type_workers"))}};
}
static json::object exact_json(const exact::Result& result) {
    const std::string status = result.replay ? (result.replay->accepted ? "INCUMBENT" : "MODEL_REJECTED") : sat::CpSolverStatus_Name(result.status);
    json::object report{{"status", status}, {"solver_status", sat::CpSolverStatus_Name(result.status)},
        {"build_seconds", result.build_seconds}, {"wall_time_seconds", result.solver_seconds},
        {"branches", result.branches}, {"conflicts", result.conflicts},
        {"proof", {{"proven", false}, {"model_infeasible", result.status == sat::INFEASIBLE},
                   {"restrictions", json::value_from(result.restrictions)}, {"relaxations", json::array{}}}}};
    if (result.replay) report["strict_replay"] = {{"accepted", result.replay->accepted}, {"strict", result.replay->strict},
        {"requirements", result.replay->requirements}, {"invariants", result.replay->invariants}, {"errors", json::value_from(result.replay->errors)}};
    return report;
}

int main(int argc, char** argv) {
    if (argc < 4) { std::cerr << "usage: early_completion_cli v3_problem.json internal_hint.json output_dir [options]\n"; return 2; }
    const fs::path output = argv[3];
    bool owns_output = false;
    try {
        early::Options options;
        for (int i = 4; i < argc; ++i) {
            const std::string flag = argv[i];
            if (flag == "--fix-source-profiles") options.fix_source_profiles = true;
            else {
                require(i + 1 < argc, "missing value for " + flag);
                const double value = std::stod(argv[++i]);
                if (flag == "--time-limit") options.seconds = value;
                else if (flag == "--screen-seconds") options.screen_seconds = value;
                else if (flag == "--exact-seconds") options.exact_seconds = value;
                else throw std::runtime_error("unsupported option " + flag);
            }
        }
        require(!fs::exists(output / "report.json") && !fs::exists(output / "candidate.json"), "output contains prior result");
        fs::create_directories(output); owns_output = true;
        const auto result = early::complete(day_solver::load_problem_json(argv[1]), read_hint(argv[2]), options);
        write_json(output / "input_hint.json", hint_json(result.ordered_hint));
        json::object report{{"status", result.accepted() ? "SCHEDULE" : "UNKNOWN"}, {"wall_time_seconds", result.seconds},
            {"model", "native_early_completion"}, {"candidate", nullptr}, {"route_hint", nullptr},
            {"coarse_report", nullptr}, {"exact_report", nullptr},
            {"scope", "Internal attempt on our generated proposal; unsuccessful restrictions do not reject the day or its owner partition"}};
        if (result.coarse) {
            const auto& coarse = *result.coarse;
            report["coarse_report"] = {{"status", coarse.hint ? "INCUMBENT" : sat::CpSolverStatus_Name(coarse.status)},
                {"solver_status", sat::CpSolverStatus_Name(coarse.status)}, {"build_seconds", coarse.build_seconds},
                {"wall_time_seconds", coarse.total_seconds}, {"solver_seconds", coarse.solver_seconds},
                {"branches", coarse.branches}, {"conflicts", coarse.conflicts}};
            if (coarse.hint) write_json(output / "route_type_hint.json", hint_json(*coarse.hint));
        }
        if (result.exact) report["exact_report"] = exact_json(*result.exact);
        if (result.accepted()) {
            write_json(output / "candidate.json", schedule_json(*result.exact->schedule));
            report["candidate"] = (output / "candidate.json").string();
            report["route_hint"] = (output / "route_type_hint.json").string();
        }
        write_json(output / "report.json", report);
        std::cout << json::serialize(report) << '\n';
        return result.accepted() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (owns_output) write_json(output / "report.json", json::object{{"status", "MODEL_REJECTED"}, {"reason", error.what()}});
        return 2;
    }
}
