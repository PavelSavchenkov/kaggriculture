#include "screen.hpp"
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
namespace screen = day_native::screen;
using namespace day_native;

static void require(bool ok, const std::string& reason) {
    if (!ok) throw std::runtime_error(reason);
}
static void write_json(const fs::path& path, const json::value& value) {
    std::ofstream stream(path);
    stream << json::serialize(value) << '\n';
    require(bool(stream), "cannot write " + path.string());
}
static int index(const json::value& value) {
    const auto number = value.as_int64();
    require(number >= std::numeric_limits<int>::min() && number <= std::numeric_limits<int>::max(), "internal hint index out of range");
    return int(number);
}
static InternalHint read_hint(const fs::path& path) {
    std::ifstream stream(path);
    require(bool(stream), "cannot read " + path.string());
    const auto document = json::parse(std::string(std::istreambuf_iterator<char>(stream), {}));
    const auto& object = document.as_object();
    InternalHint hint;
    for (const auto& value : object.at("task_assignments").as_array()) {
        const auto& row = value.as_object();
        hint.assignments.push_back({index(row.at("task")), index(row.at("worker")), index(row.at("hour")), {}, {}});
    }
    if (const auto* groups = object.if_contains("type_workers")) {
        hint.type_workers.emplace();
        for (const auto& group : groups->as_array()) {
            std::vector<int> members;
            for (const auto& member : group.as_array()) members.push_back(index(member));
            hint.type_workers->push_back(std::move(members));
        }
    }
    return hint;
}
static json::value hint_json(const InternalHint& hint) {
    json::array assignments;
    for (const auto& value : hint.assignments) assignments.emplace_back(json::object{
        {"task", value.task}, {"worker", value.worker}, {"hour", required(value.hour, "hour")}, {"type", required(value.type, "type")}});
    return json::object{{"task_assignments", assignments}, {"type_workers", json::value_from(required(hint.type_workers, "type_workers"))}};
}
static json::array profiles_json(const std::vector<screen::Profile>& profiles) {
    json::array result;
    for (const auto& profile : profiles) result.push_back({json::value_from(profile.starts), profile.release});
    return result;
}
static json::object data_json(const screen::DataSnapshot& data) {
    json::array tasks, requirements, deliveries, net;
    for (const auto& task : data.tasks) tasks.push_back({
        {"id", task.id}, {"tile", task.tile}, {"point", json::value_from(task.point)},
        {"input_item", task.input}, {"seed_crop", task.crop}, {"output_item", task.output},
        {"output_quantity", task.quantity}, {"predecessor", task.predecessor},
        {"early", data.early[task.id]}, {"late", data.late[task.id]}});
    for (const auto& [key, quantity] : data.requirements) requirements.push_back({key.first, key.second, quantity});
    for (const auto& [key, ids] : data.deliveries) deliveries.push_back({key.first, key.second, json::value_from(ids)});
    for (const auto& [key, quantity] : data.net) net.push_back({key.first, key.second, quantity});
    return {{"tasks", tasks}, {"requirements", requirements}, {"deliveries", deliveries}, {"net", net},
        {"deadlines", json::value_from(data.deadlines)}, {"owner", json::value_from(data.owner)},
        {"route_ids", json::value_from(data.route_ids)}, {"workers", profiles_json(data.workers)},
        {"profiles", profiles_json(data.profiles)}, {"profile_workers", json::value_from(data.groups)},
        {"worker_group", json::value_from(data.worker_group)}};
}
static json::object report_json(const screen::Result& result, const screen::ScreenOptions& options, const fs::path& output) {
    json::object report{{"status", "BUILT"}};
    if (result.solved) {
        json::object pickups;
        for (const auto& [route, needs] : result.pickup_requirements) {
            json::object quantities;
            for (const auto& [item, tasks] : needs) quantities[std::to_string(item)] = json::value_from(tasks);
            pickups[std::to_string(route)] = quantities;
        }
        report = {{"status", sat::CpSolverStatus_Name(result.status)}, {"solver_status", sat::CpSolverStatus_Name(result.status)},
            {"solver_seconds", result.solver_seconds}, {"branches", result.branches}, {"conflicts", result.conflicts},
            {"routes", result.routes}, {"cross_route_precedence_relaxed", options.ignore_precedence},
            {"cross_route_precedence_softened", options.soft_precedence}, {"availability_softened", options.soft_availability},
            {"pickup_requirements", pickups}, {"initial_spawns_coupled", result.initial_spawns_coupled},
            {"hire_checkpoints_coupled", result.hire_checkpoints_coupled}};
        if (result.hint) {
            if (result.worker_start_profiles) report["worker_start_profiles"] = profiles_json(*result.worker_start_profiles);
            json::array violations, deficits, deliveries;
            for (const auto& value : result.violations) violations.push_back({
                {"predecessor", value.predecessor}, {"successor", value.successor}, {"predecessor_route", value.predecessor_route},
                {"successor_route", value.successor_route}, {"predecessor_hour", value.predecessor_hour}, {"successor_hour", value.successor_hour}});
            for (const auto& value : result.deficits) deficits.push_back({{"item", value.item}, {"deadline", value.deadline},
                {"task", value.task ? json::value(*value.task) : json::value(nullptr)}, {"quantity", value.quantity}});
            for (const auto& value : result.deliveries) {
                json::object by_deadline;
                for (auto [hour, quantity] : value.delivered_by) by_deadline[std::to_string(hour)] = quantity;
                deliveries.push_back({{"task", value.task}, {"item", value.item}, {"route", value.route}, {"hour", value.hour},
                    {"quantity", value.quantity}, {"delivered", value.delivered}, {"delivered_by", by_deadline}});
            }
            report["cross_route_violations"] = violations; report["availability_deficits"] = deficits; report["delivery_task_profiles"] = deliveries;
            report["status"] = "INCUMBENT"; report["hint"] = (output / "route_type_hint.json").string();
        }
    }
    report["model"] = "native_v3_coarse_route_screen";
    report["scope"] = "Internal route relaxation; not a replay-validated day schedule";
    report["build_seconds"] = result.build_seconds; report["wall_time_seconds"] = result.total_seconds;
    report["variables"] = result.variables; report["constraints"] = result.constraints;
    return report;
}

int main(int argc, char** argv) {
    if (argc < 4) { std::cerr << "usage: native_screen_cli problem.json internal_owner_hint.json output_dir [options]\n"; return 2; }
    const fs::path output = argv[3];
    bool owns_output = false;
    try {
        screen::SolveOptions settings;
        auto& options = settings.model;
        fs::path export_path, data_path;
        for (int index = 4; index < argc; ++index) {
            const std::string flag = argv[index];
            if (flag == "--ignore-availability") options.ignore_availability = true;
            else if (flag == "--ignore-pickups") options.ignore_pickups = true;
            else if (flag == "--ignore-cross-route-precedence") options.ignore_precedence = true;
            else if (flag == "--soft-cross-route-precedence") options.soft_precedence = true;
            else if (flag == "--soft-availability") options.soft_availability = true;
            else if (flag == "--fix-source-order") options.fixed_order = true;
            else if (flag == "--fix-source-profile") options.fixed_profile = true;
            else if (flag == "--build-only") settings.build_only = true;
            else {
                require(index + 1 < argc, "missing value for " + flag);
                const std::string value = argv[++index];
                if (flag == "--time-limit") settings.seconds = std::stod(value);
                else if (flag == "--workers") settings.workers = std::stoi(value);
                else if (flag == "--seed") settings.seed = std::stoi(value);
                else if (flag == "--free-source-order-route") options.free_order.insert(std::stoi(value));
                else if (flag == "--late-starts") { require(value == "static" || value == "dynamic", "invalid late-start mode"); options.dynamic = value == "dynamic"; }
                else if (flag == "--availability-mode") { require(value == "aggregate" || value == "fixed", "invalid availability mode"); options.fixed_availability = value == "fixed"; }
                else if (flag == "--export-model") { export_path = value; settings.include_model = true; }
                else if (flag == "--dump-data") { data_path = value; settings.include_data = true; }
                else throw std::runtime_error("unsupported option " + flag);
            }
        }
        require(std::isfinite(settings.seconds) && settings.seconds > 0 && settings.workers > 0, "positive search limits required");
        require(!(options.ignore_precedence && options.soft_precedence), "precedence cannot be both ignored and softened");
        require(!fs::exists(output / "report.json") && !fs::exists(output / "route_type_hint.json"), "output already contains a result");
        fs::create_directories(output); owns_output = true;
        const auto problem = ds::load_problem_json(argv[1]);
        const auto result = screen::solve(problem, read_hint(argv[2]), settings);
        if (!data_path.empty()) write_json(data_path, data_json(*result.data));
        if (!export_path.empty()) {
            std::string text;
            require(google::protobuf::TextFormat::PrintToString(*result.model, &text), "cannot serialize coarse model");
            std::ofstream file(export_path); file << text; require(bool(file), "cannot write coarse model");
        }
        if (result.hint) write_json(output / "route_type_hint.json", hint_json(*result.hint));
        const auto report = report_json(result, options, output);
        write_json(output / "report.json", report); std::cout << json::serialize(report) << '\n';
        return report.at("status") == "INCUMBENT" || report.at("status") == "BUILT" ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (owns_output) write_json(output / "report.json", json::object{{"status", "MODEL_REJECTED"}, {"reason", error.what()}});
        return 2;
    }
}
