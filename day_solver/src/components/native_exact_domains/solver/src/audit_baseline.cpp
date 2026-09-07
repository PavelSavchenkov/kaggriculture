#include <cstdio>
#include <exception>
#include <iostream>
#include <string_view>

#include <boost/json.hpp>

#include "baseline_json.hpp"
#include "problem_json.hpp"
#include "replay.hpp"

namespace {

namespace json = boost::json;

template <class T, size_t N>
json::array fixed_array(const std::array<T, N>& values) {
    json::array result;
    for (T value : values) result.push_back(static_cast<int64_t>(value));
    return result;
}

json::object physical_state(const day_solver::PhysicalState& state) {
    json::array tiles;
    for (const auto& tile : state.managed_tiles) {
        const auto& value = tile.state;
        tiles.push_back({
            {"x", tile.x}, {"y", tile.y},
            {"kind", static_cast<uint8_t>(value.kind)},
            {"crop", value.crop}, {"animal", value.animal},
            {"age_days", value.age_days},
            {"stored_units", value.stored_units},
            {"consecutive_dry_days", value.consecutive_dry_days},
            {"pending_care_bonus", value.pending_care_bonus},
            {"fertilizer_days_remaining", value.fertilizer_days_remaining},
            {"watered_today", value.watered_today},
            {"fed_today", value.fed_today},
            {"cared_today", value.cared_today},
            {"fertilizer_available", value.fertilizer_available},
        });
    }
    return {
        {"managed_tiles", std::move(tiles)},
        {"shed", fixed_array(state.shed)},
        {"seeds", fixed_array(state.seeds)},
        {"shed_capacity", state.shed_capacity},
        {"cash", state.cash},
    };
}

json::object end_state(const day_solver::DayEndState& state) {
    json::array workers;
    for (const auto& worker : state.workers) {
        json::array order;
        for (uint8_t item : worker.cargo_order) order.push_back(item);
        workers.push_back({
            {"x", worker.x}, {"y", worker.y},
            {"cargo", fixed_array(worker.cargo)},
            {"cargo_order", std::move(order)},
        });
    }
    return {
        {"physical", physical_state(state.physical)},
        {"workers", std::move(workers)},
    };
}

json::array outcome_values(
        const std::vector<day_solver::OutcomeValue>& values) {
    json::array result;
    for (const auto& outcome : values) {
        result.push_back({
            {"metric", static_cast<uint8_t>(outcome.key.metric)},
            {"subject", outcome.key.subject},
            {"tile", outcome.key.tile},
            {"through_hour", outcome.key.through_hour},
            {"value", outcome.value},
        });
    }
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::fprintf(stderr,
            "usage: audit_baseline PROBLEM.json BASELINE.json "
            "[--json|--json-trace]\n");
        return 2;
    }
    const bool json_output = argc == 4 &&
        (std::string_view(argv[3]) == "--json" ||
         std::string_view(argv[3]) == "--json-trace");
    const bool json_trace = argc == 4 &&
        std::string_view(argv[3]) == "--json-trace";
    if (argc == 4 && !json_output) {
        std::fprintf(stderr,
            "third argument must be --json or --json-trace\n");
        return 2;
    }
    try {
        const auto problem = day_solver::load_problem_json(argv[1]);
        const auto actions = day_solver::load_baseline_json(argv[2]);
        const auto replay = day_solver::replay_schedule(problem, actions);
        if (json_output) {
            namespace json = boost::json;
            json::object root;
            root["strict"] = replay.candidate.replay.strict_valid;
            root["requirements"] = replay.requirements_satisfied;
            root["invariants"] = replay.invariants_satisfied;
            json::object costs;
            costs["unit_actions"] = replay.candidate.costs.unit_actions;
            costs["travel_actions"] = replay.candidate.costs.travel_actions;
            costs["hires"] = replay.candidate.costs.hires;
            costs["purchased_units"] = replay.candidate.costs.purchased_units;
            costs["acquisition_cost"] = replay.candidate.costs.acquisition_cost;
            costs["sold_units"] = replay.candidate.costs.sold_units;
            costs["sale_proceeds"] = replay.candidate.costs.sale_proceeds;
            costs["pickups"] = replay.candidate.costs.pickups;
            costs["drops"] = replay.candidate.costs.drops;
            costs["spare_turns"] = replay.candidate.costs.spare_turns;
            root["costs"] = std::move(costs);
            json::array availability;
            for (const auto& target : replay.candidate.sale_availability) {
                json::object item;
                item["market_event"] = target.market_event;
                item["item"] = target.item;
                json::array hours;
                for (int hour : target.unit_hours) hours.push_back(hour);
                item["unit_hours"] = std::move(hours);
                availability.push_back(std::move(item));
            }
            root["sale_availability"] = std::move(availability);
            if (json_trace) {
                json::array hours;
                for (const auto& summary : replay.candidate.hours) {
                    json::object hour;
                    hour["hour"] = summary.hour;
                    json::array after_workers;
                    for (day_solver::InventoryCount count : summary.shed_after_workers)
                        after_workers.push_back(count);
                    hour["shed_after_workers"] = std::move(after_workers);
                    json::array after_market;
                    for (day_solver::InventoryCount count : summary.shed_after_market)
                        after_market.push_back(count);
                    hour["shed_after_market"] = std::move(after_market);
                    hour["headroom_after_workers"] =
                        summary.headroom_after_workers;
                    hour["headroom_after_market"] =
                        summary.headroom_after_market;
                    hour["cumulative_outcomes"] = outcome_values(
                        summary.cumulative_outcomes
                    );
                    hours.push_back(std::move(hour));
                }
                root["hours"] = std::move(hours);
            }
            root["outcomes"] = outcome_values(replay.candidate.outcomes);
            root["schedule_hash"] = replay.candidate.replay.schedule_hash;
            root["end_state_hash"] = replay.candidate.replay.end_state_hash;
            root["end_state"] = end_state(replay.candidate.end);
            json::array errors;
            for (const auto& error : replay.errors)
                errors.push_back(json::value(error));
            root["errors"] = std::move(errors);
            std::cout << json::serialize(root) << '\n';
        } else {
            std::printf(
                "strict=%d requirements=%d invariants=%d unit_actions=%u "
                "travel=%u hires=%u purchases=%llu acquisition_cost=%lld errors=%zu\n",
                replay.candidate.replay.strict_valid,
                replay.requirements_satisfied, replay.invariants_satisfied,
                replay.candidate.costs.unit_actions,
                replay.candidate.costs.travel_actions,
                replay.candidate.costs.hires,
                static_cast<unsigned long long>(replay.candidate.costs.purchased_units),
                static_cast<long long>(replay.candidate.costs.acquisition_cost),
                replay.errors.size());
        }
        for (const auto& error : replay.errors)
            std::fprintf(stderr, "%s\n", error.c_str());
        return replay.candidate.replay.strict_valid &&
            replay.requirements_satisfied && replay.invariants_satisfied ? 0 : 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }
}
