#pragma once
#include "routing_data.hpp"

namespace day_constructor {
template <class Values> json::array numbers(const Values& values) {
    json::array result;
    for (auto value : values) result.push_back(int64_t(value));
    return result;
}
template <class Values> json::array pairs(const Values& values) {
    json::array result;
    for (auto [a, b] : values) result.push_back(json::array{a, b});
    return result;
}
inline json::object dump(const RoutingData& routing, const pyvrp::ProblemData& model) {
    const auto& td = routing.task_data;
    json::object result{{"early", numbers(td.early)}, {"late", numbers(td.late)},
                        {"free_inputs", numbers(routing.free_inputs)}, {"input_ready", pairs(routing.input_ready)},
                        {"late_inputs", pairs(routing.late_inputs)}, {"late_seeds", pairs(routing.late_seeds)},
                        {"resource_pairs", pairs(routing.resource_pairs)}, {"shipments_tasks", pairs(routing.shipment_tasks)},
                        {"hires", numbers(td.hires)}, {"input_items", numbers(routing.input_items)}};
    json::array tasks, deliveries, groups;
    for (const auto& task : td.tasks)
        tasks.push_back(json::array{task.id, task.pattern, task.tile, task.point[0], task.point[1], task.input, task.crop, task.output, task.quantity, task.predecessor});
    for (const auto& [key, ids] : td.deliveries) deliveries.push_back(json::array{key.first, key.second, numbers(ids)});
    for (const auto& group : routing.client_tasks) groups.push_back(numbers(group));
    result["tasks"] = tasks; result["deliveries"] = deliveries; result["client_tasks"] = groups;
    json::array locations, clients, depots, shipments, vehicles, distances, durations;
    for (const auto& location : model.locations()) locations.push_back(json::array{double(location.x), double(location.y), location.name});
    for (const auto& client : model.clients()) {
        clients.push_back(json::object{
            {"location", client.location}, {"delivery", numbers(client.delivery)}, {"pickup", numbers(client.pickup)},
            {"service_duration", int64_t(client.serviceDuration)}, {"tw_early", int64_t(client.twEarly)}, {"tw_late", int64_t(client.twLate)},
            {"release_time", int64_t(client.releaseTime)}, {"prize", int64_t(client.prize)}, {"required", client.required},
            {"group", client.group ? json::value(*client.group) : json::value(nullptr)}, {"name", client.name}});
    }
    for (const auto& depot : model.depots()) depots.push_back(json::object{
        {"location", depot.location}, {"tw_early", int64_t(depot.twEarly)}, {"tw_late", int64_t(depot.twLate)},
        {"service_duration", int64_t(depot.serviceDuration)}, {"name", depot.name}});
    auto step = [](const auto& value) { return json::object{
        {"location", value.location}, {"tw_early", int64_t(value.twEarly)}, {"tw_late", int64_t(value.twLate)},
        {"service_duration", int64_t(value.serviceDuration)}}; };
    for (const auto& shipment : model.shipments()) shipments.push_back(json::object{
        {"pickup", step(shipment.pickup)}, {"delivery", step(shipment.delivery)}, {"amount", numbers(shipment.amount)},
        {"prize", int64_t(shipment.prize)}, {"required", shipment.required}, {"name", shipment.name}});
    for (const auto& v : model.vehicleTypes()) vehicles.push_back(json::object{
        {"num_available", v.numAvailable}, {"start_depot", v.startDepot}, {"end_depot", v.endDepot}, {"capacity", numbers(v.capacity)},
        {"tw_early", int64_t(v.twEarly)}, {"tw_late", int64_t(v.twLate)}, {"shift_duration", int64_t(v.shiftDuration)},
        {"max_distance", int64_t(v.maxDistance)}, {"fixed_cost", int64_t(v.fixedCost)}, {"unit_distance_cost", int64_t(v.unitDistanceCost)},
        {"unit_duration_cost", int64_t(v.unitDurationCost)}, {"profile", v.profile}, {"start_late", int64_t(v.startLate)},
        {"initial_load", numbers(v.initialLoad)}, {"reload_depots", numbers(v.reloadDepots)}, {"max_reloads", v.maxReloads},
        {"max_overtime", int64_t(v.maxOvertime)}, {"unit_overtime_cost", int64_t(v.unitOvertimeCost)}, {"max_duration", int64_t(v.maxDuration)}, {"name", v.name}});
    auto matrix = [&](const auto& value) {
        json::array rows;
        for (size_t i = 0; i < model.locations().size(); ++i) {
            json::array row;
            for (size_t j = 0; j < model.locations().size(); ++j) row.push_back(int64_t(value(i, j)));
            rows.push_back(row);
        }
        return rows;
    };
    for (const auto& value : model.distanceMatrices()) distances.push_back(matrix(value));
    for (const auto& value : model.durationMatrices()) durations.push_back(matrix(value));
    result["model"] = json::object{{"locations", locations}, {"clients", clients}, {"depots", depots}, {"shipments", shipments},
                                    {"vehicles", vehicles}, {"distances", distances}, {"durations", durations}, {"groups", json::array{}}};
    return result;
}
}  // namespace day_constructor
