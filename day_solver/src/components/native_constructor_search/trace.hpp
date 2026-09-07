#pragma once
#include "search.hpp"
#include "dump_data.hpp"
#include <boost/uuid/detail/sha1.hpp>
#include <iomanip>
#include <sstream>

namespace day_constructor {
inline json::array solution_routes(const vrp::Solution& solution) {
    json::array routes;
    for (const auto& route : solution.routes()) {
        json::array activities;
        for (const auto& activity : route.schedule()) activities.push_back(json::array{
            int(activity.type()), activity.idx(), activity.trip(), Count(activity.startTime()), Count(activity.endTime()),
            Count(activity.waitDuration()), Count(activity.timeWarp())});
        routes.push_back(json::array{route.vehicleType(), activities});
    }
    json::array unplanned;
    for (const auto& activity : solution.unplanned()) unplanned.push_back(json::array{int(activity.type()), activity.idx()});
    return {routes, unplanned};
}
inline std::string signature(const vrp::Solution& solution) {
    const auto text = json::serialize(solution_routes(solution));
    boost::uuids::detail::sha1 hash;
    hash.process_bytes(text.data(), text.size());
    boost::uuids::detail::sha1::digest_type digest;
    hash.get_digest(digest);
    std::ostringstream output;
    for (auto word : digest) output << std::hex << std::setw(8) << std::setfill('0') << word;
    return output.str();
}
inline json::object solution_summary(const vrp::Solution& solution, const vrp::CostEvaluator& evaluator) {
    return { {"signature", signature(solution)}, {"cost", Count(evaluator.cost(solution))},
             {"penalised_cost", Count(evaluator.penalisedCost(solution))}, {"feasible", solution.isFeasible()},
             {"distance", Count(solution.distance())}, {"time_warp", Count(solution.timeWarp())},
             {"excess_load", numbers(solution.excessLoad())}, {"excess_distance", Count(solution.excessDistance())} };
}
inline json::array neighbour_data(const Search& search) {
    std::map<std::pair<int, int>, json::array> sorted;
    for (const auto& [activity, nearby] : search.neighbours()) {
        json::array values;
        for (const auto& value : nearby) values.push_back(json::array{int(value.type()), value.idx()});
        sorted[{int(activity.type()), activity.idx()}] = values;
    }
    json::array result;
    for (const auto& [key, values] : sorted) result.push_back(json::array{key.first, key.second, values});
    return result;
}
}  // namespace day_constructor
