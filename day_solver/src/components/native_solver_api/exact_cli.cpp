#include "exact.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <boost/json.hpp>
#include "google/protobuf/text_format.h"
#include "problem_json.hpp"

namespace ds = day_solver;
namespace sat = operations_research::sat;
namespace json = boost::json;
namespace fs = std::filesystem;
namespace exact = day_native::exact;
using namespace day_native;
constexpr int hours = ds::HOURS;

static void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
#include "json_output.hpp"

static int index(const json::value& value) {
    const auto number = value.as_int64();
    require(number >= std::numeric_limits<int>::min() && number <= std::numeric_limits<int>::max(), "internal hint index out of range");
    return int(number);
}

static InternalHint read_hint(const fs::path& path, HintKind kind) {
    std::ifstream stream(path);
    require(bool(stream), "cannot read internal hint");
    const auto document = json::parse(std::string(std::istreambuf_iterator<char>(stream), {}));
    const auto& object = document.as_object();
    const bool partial = kind == HintKind::partial || kind == HintKind::fixed_partial;
    const bool concrete = kind == HintKind::route_task;
    InternalHint hint;
    for (const auto& value : object.at("task_assignments").as_array()) {
        const auto& row = value.as_object();
        HintAssignment assignment{index(row.at("task")), index(row.at("worker")), {}, {}, {}};
        if (kind != HintKind::route_owner) assignment.hour = index(row.at("hour"));
        if (!partial && !concrete) assignment.type = index(row.at("type"));
        if (kind == HintKind::chain_type && row.if_contains("chain")) assignment.chain = index(row.at("chain"));
        hint.assignments.push_back(assignment);
    }
    if (!partial) {
        if (object.if_contains("partial_routes")) hint.partial_routes = object.at("partial_routes").as_bool();
        if (!concrete) {
            hint.type_workers.emplace();
            for (const auto& group : object.at("type_workers").as_array()) {
                std::vector<int> workers;
                for (const auto& member : group.as_array()) workers.push_back(index(member));
                hint.type_workers->push_back(std::move(workers));
            }
        }
    }
    return hint;
}

static json::object report_json(const exact::Result& result, const exact::HintOptions& hints,
                                const std::map<HintKind, fs::path>& paths) {
    json::object report{{"status", "BUILT"}, {"model", "native_v3_time_expanded"}, {"tasks", result.tasks},
        {"workers", result.workers}, {"variables", result.variables}, {"constraints", result.constraints},
        {"pruned_task_variables", result.pruned_task_variables}, {"build_seconds", result.build_seconds}};
    if (!result.solved) return report;
    report["status"] = sat::CpSolverStatus_Name(result.status);
    report["solver_status"] = sat::CpSolverStatus_Name(result.status);
    report["wall_time_seconds"] = result.solver_seconds;
    report["branches"] = result.branches; report["conflicts"] = result.conflicts;
    report["proof"] = {{"proven", false}, {"model_infeasible", result.status == sat::INFEASIBLE},
        {"scope", result.restrictions.empty() ? "all task assignments, movements, inventory actions" : "restricted hinted model"},
        {"restrictions", json::value_from(result.restrictions)}, {"relaxations", json::array{}}};
    for (const auto& [kind, path] : paths) {
        auto key = std::string(hint_name(kind)).substr(2);
        std::replace(key.begin(), key.end(), '-', '_');
        report[key] = path.string();
    }
    report["free_fixed_routes"] = json::value_from(hints.free_routes);
    for (const auto& [name, identities] : result.cores) {
        json::array core;
        for (const auto& identity : identities) core.push_back(std::visit([](const auto& value) { return json::value_from(value); }, identity));
        report[name] = std::move(core);
    }
    if (result.replay) {
        const auto& replay = *result.replay;
        report["strict_replay"] = {{"accepted", replay.accepted}, {"strict", replay.strict},
            {"requirements", replay.requirements}, {"invariants", replay.invariants}, {"errors", json::value_from(replay.errors)}};
        report["status"] = replay.accepted ? "INCUMBENT" : "MODEL_REJECTED";
    }
    return report;
}

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "usage: native_exact_cli problem.json output_dir [options]\n"; return 2; }
    const fs::path output = argv[2];
    bool owns_output = false;
    try {
        exact::SolveOptions options;
        exact::HintOptions hints;
        std::map<HintKind, fs::path> paths;
        fs::path export_path;
        for (int argument = 3; argument < argc; ++argument) {
            const std::string option = argv[argument];
            bool parsed = false;
            for (auto flag : {HintFlag::fix_partial, HintFlag::diagnose_partial, HintFlag::diagnose_task_route,
                              HintFlag::diagnose_type_route, HintFlag::diagnose_owner})
                if (option == flag_name(flag)) { hints.flags.insert(flag); parsed = true; break; }
            if (parsed) continue;
            if (option == "--build-only") { options.build_only = true; continue; }
            if (option == "--log-search-progress") { options.log_search = true; continue; }
            require(argument + 1 < argc, "missing value for " + option);
            const std::string value = argv[++argument];
            for (auto kind : {HintKind::partial, HintKind::fixed_partial, HintKind::route_task, HintKind::route_type, HintKind::route_owner, HintKind::chain_type})
                if (option == hint_name(kind)) {
                    require(!paths.contains(kind), "duplicate hint option");
                    paths[kind] = value; parsed = true; break;
                }
            if (parsed) continue;
            if (option == "--time-limit") options.seconds = std::stod(value);
            else if (option == "--workers") options.workers = std::stoi(value);
            else if (option == "--export-model") { export_path = value; options.include_model = true; }
            else if (option == "--search-objective") {
                require(value == "none" || value == "earliest", "invalid search objective");
                options.earliest = value == "earliest";
            } else if (option == "--free-fixed-route" || option == "--free-partial-task") {
                const int number = std::stoi(value);
                require(number >= 0, "negative free task/route");
                (option == "--free-fixed-route" ? hints.free_routes : hints.free_tasks).insert(number);
            } else throw std::runtime_error("unsupported option " + option);
        }
        require(std::isfinite(options.seconds) && options.seconds > 0 && options.workers > 0, "positive search budget and worker count required");
        require(!fs::exists(output / "report.json") && !fs::exists(output / "candidate.json"), "output already contains a result");
        fs::create_directories(output); owns_output = true;
        const auto problem = ds::load_problem_json(argv[1]);
        for (const auto& [kind, path] : paths) hints.documents[kind] = read_hint(path, kind);
        const auto result = exact::solve(problem, hints, options);
        if (!export_path.empty()) {
            std::string text;
            require(google::protobuf::TextFormat::PrintToString(*result.model, &text), "cannot serialize model");
            std::ofstream destination(export_path); destination << text;
            require(bool(destination), "cannot write model");
        }
        const auto report = report_json(result, hints, paths);
        if (result.schedule) write_json(output / "candidate.json", schedule_json(*result.schedule));
        write_json(output / "report.json", report);
        std::cout << json::serialize(report) << '\n';
        return report.at("status") == "INCUMBENT" || report.at("status") == "BUILT" ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (owns_output) write_json(output / "report.json", json::object{
            {"status", "MODEL_REJECTED"}, {"reason", error.what()}, {"proof", json::object{{"proven", false}}}});
        return 2;
    }
}
