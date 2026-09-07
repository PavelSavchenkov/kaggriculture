// Test adapter only. Scripted accepted results are NOT replay evidence.
#include "adapter.hpp"
#include <iostream>
using namespace semantic_adapter;

static double numeric(const json::value& value) { return value.is_double() ? value.as_double() : double(value.as_int64()); }
static sat::CpSolverStatus status(const json::object& row) {
    sat::CpSolverStatus result;
    const auto value = row.at("solver_status").as_string();
    require(sat::CpSolverStatus_Parse(std::string(value), &result), "invalid scripted status");
    return result;
}
int main(int argc, char** argv) {
    try {
        require(argc == 5, "usage: scripted_control problem hint script output");
        const auto config = read_json(argv[3]).as_object();
        const auto responses = config.at("responses").as_object();
        double now = 0;
        json::array calls, results;
        std::map<std::string, int> counts;
        auto response = [&](std::string_view label, const std::string& kind) {
            const std::string name(label);
            const auto* entry = responses.if_contains(name);
            if (!entry) entry = responses.if_contains("default_" + kind);
            require(entry, "missing script response " + name);
            json::object row;
            if (entry->is_array()) {
                const auto& array = entry->as_array();
                row = array.at(std::min(std::size_t(counts[name]++), array.size() - 1)).as_object();
            } else row = entry->as_object();
            if (const auto* elapsed = row.if_contains("elapsed")) now += numeric(*elapsed);
            return row;
        };
        semantic::Backends backends{
            [&](std::string_view label, const auto&, const InternalHint& hint, const screen::SolveOptions& settings) {
                require(settings.model.dynamic && settings.model.fixed_order && !settings.model.ignore_availability &&
                    !settings.model.ignore_pickups && !settings.model.ignore_precedence && !settings.model.fixed_availability &&
                    settings.model.free_order.empty() && !settings.include_model && !settings.include_data && !settings.build_only,
                    "unexpected coarse relaxation or option");
                require(settings.model.soft_precedence == settings.model.soft_availability, "unequal soft flags");
                calls.push_back({{"name", std::string(label)}, {"kind", "screen"}, {"hint", hint_json(hint)},
                    {"seconds", settings.seconds}, {"workers", settings.workers}, {"seed", settings.seed},
                    {"soft", settings.model.soft_precedence}, {"fix_profiles", settings.model.fixed_profile}});
                auto row = response(label, "screen");
                if (auto* reason = row.if_contains("throw")) throw std::runtime_error(std::string(reason->as_string()));
                screen::Result result; result.solved = true; result.status = status(row);
                if (result.status == sat::OPTIMAL || result.status == sat::FEASIBLE) {
                    result.hint = row.contains("hint") ? read_hint(row.at("hint")) : hint;
                    if (auto* rows = row.if_contains("cross_route_violations")) for (const auto& value : rows->as_array()) {
                        const auto& v = value.as_object();
                        result.violations.push_back({index(v.at("predecessor")), index(v.at("successor")), 0, 0, 0, 0});
                    }
                    if (auto* rows = row.if_contains("availability_deficits")) for (const auto& value : rows->as_array()) {
                        const auto& v = value.as_object();
                        result.deficits.push_back({index(v.at("item")), index(v.at("deadline")), {}, v.at("quantity").as_int64()});
                    }
                    if (auto* rows = row.if_contains("delivery_task_profiles")) for (const auto& value : rows->as_array()) {
                        const auto& v = value.as_object();
                        screen::Delivery delivery{0, index(v.at("item")), index(v.at("route")), 0, v.at("quantity").as_int64(), 0, {}};
                        for (const auto& entry : v.at("delivered_by").as_object()) delivery.delivered_by[std::stoi(std::string(entry.key()))] = entry.value().as_int64();
                        result.deliveries.push_back(std::move(delivery));
                    }
                }
                return result;
            },
            [&](std::string_view label, const auto&, const exact::HintOptions& hints, const exact::SolveOptions& settings) {
                require(hints.documents.size() == 1 && hints.flags.empty() && hints.free_tasks.empty() &&
                    !settings.build_only && !settings.include_model && !settings.earliest && !settings.log_search,
                    "unexpected exact restriction or option");
                const auto& [kind, hint] = *hints.documents.begin();
                const std::string name = kind == HintKind::route_owner ? "owner" : kind == HintKind::route_type ? "type" : "partial";
                calls.push_back({{"name", std::string(label)}, {"kind", "exact"}, {"hint", hint_json(hint)},
                    {"seconds", settings.seconds}, {"workers", settings.workers}, {"restriction", name},
                    {"free_routes", json::value_from(hints.free_routes)}});
                auto row = response(label, "exact");
                if (auto* reason = row.if_contains("throw")) throw std::runtime_error(std::string(reason->as_string()));
                exact::Result result; result.solved = true; result.status = status(row);
                result.restrictions = {kind == HintKind::route_owner ? "fixed_route_owner_type_hint" : kind == HintKind::route_type ? "fixed_route_type_hint" : "fixed_partial_hint"};
                if (auto* restrictions = row.if_contains("restrictions")) result.restrictions = json::value_to<std::vector<std::string>>(*restrictions);
                if (result.status == sat::OPTIMAL || result.status == sat::FEASIBLE) {
                    const bool accepted = !row.contains("accepted") || row.at("accepted").as_bool();
                    result.schedule.emplace();
                    result.replay = exact::ReplayCheck{accepted, accepted, accepted, accepted, {}};
                }
                return result;
            }, [&] { return now; }};
        semantic::Session session(day_solver::load_problem_json(argv[1]), backends);
        const auto original = read_hint(read_json(argv[2]));
        for (const auto& value : config.at("runs").as_array()) {
            const auto& run = value.as_object();
            semantic::Options options;
            for (const auto& entry : run.at("options").as_object()) set_option(options, std::string(entry.key()), numeric(entry.value()));
            const auto hint = run.contains("hint") ? read_hint(run.at("hint")) : original;
            auto result = session.repair(hint, options);
            auto report = result_json(result);
            report["scope"] = "Scripted control flow only; no replay claim";
            results.push_back(report);
        }
        write_json(argv[4], json::object{{"calls", calls}, {"results", results}});
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
