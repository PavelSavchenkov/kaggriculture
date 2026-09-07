#pragma once

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"
namespace day_native::ready_screen {
namespace sat = operations_research::sat;
using operations_research::Domain;
using sat::BoolVar;
using sat::IntVar;
using sat::LinearExpr;

IntVar integer(sat::CpModelBuilder& model, Count lower, Count upper, const std::string& label) {
    return model.NewIntVar(Domain(lower, upper)).WithName(label);
}
BoolVar boolean(sat::CpModelBuilder& model, const std::string& label) { return model.NewBoolVar().WithName(label); }
LinearExpr point_distance(sat::CpModelBuilder& model, IntVar x, IntVar y,
                          LinearExpr px, LinearExpr py, const std::string& label) {
    const auto dx = integer(model, 0, 9, label + ".dx"), dy = integer(model, 0, 9, label + ".dy");
    model.AddAbsEquality(dx, x - px); model.AddAbsEquality(dy, y - py);
    return dx + dy;
}

struct SpawnCheckpoints {
    sat::CpModelBuilder& model;
    std::vector<int> releases, times;
    std::vector<std::map<int, IntVar>> x, y;
    SpawnCheckpoints(sat::CpModelBuilder& model, const std::vector<int>& hires) : model(model), releases{0}, x(hires.size() + 1), y(x.size()) {
        for (int hour : hires) releases.push_back(hour + 1);
        std::set<int> unique(releases.begin(), releases.end()); times.assign(unique.begin(), unique.end());
        for (int worker = 0; worker < int(releases.size()); ++worker) {
            std::vector<int> active_times;
            for (int hour : times) if (hour >= releases[worker]) active_times.push_back(hour);
            for (int hour : active_times) x[worker][hour] = integer(model, 0, 9, name("checkpoint_x", {worker, hour}));
            for (int hour : active_times) y[worker][hour] = integer(model, 0, 9, name("checkpoint_y", {worker, hour}));
            for (std::size_t i = 1; i < active_times.size(); ++i) {
                const int first = active_times[i - 1], second = active_times[i];
                auto travel = point_distance(model, x[worker].at(first), y[worker].at(first), x[worker].at(second), y[worker].at(second),
                                             name("checkpoint_travel", {worker, first, second}));
                model.AddLessOrEqual(travel, second - first);
            }
        }
        model.AddEquality(x[0].at(0), shed[0][0]); model.AddEquality(y[0].at(0), shed[0][1]);
        int active = 1;
        for (int hour = 0; hour < hours; ++hour) {
            const int count = std::count(hires.begin(), hires.end(), hour);
            if (!count) continue;
            std::array<LinearExpr, 4> occupancy;
            for (int point = 0; point < 4; ++point)
                for (int worker = 0; worker < active; ++worker) {
                    auto literal = boolean(model, name("hire_occ", {hour, worker, point}));
                    auto table = model.AddAllowedAssignments({x[worker].at(hour + 1), y[worker].at(hour + 1), literal});
                    for (int px = 0; px < 10; ++px) for (int py = 0; py < 10; ++py)
                        table.AddTuple({px, py, int(shed[point] == Point{px, py})});
                    occupancy[point] += literal;
                }
            for (int index = 0; index < count; ++index) {
                const int worker = active++;
                std::vector<BoolVar> choices;
                for (int point = 0; point < 4; ++point) choices.push_back(boolean(model, name("spawn", {worker, point})));
                model.AddExactlyOne(choices);
                for (int point = 0; point < 4; ++point) {
                    for (int other = 0; other < 4; ++other) {
                        if (other < point) model.AddLessThan(occupancy[point], occupancy[other]).OnlyEnforceIf(choices[point]);
                        if (other > point) model.AddLessOrEqual(occupancy[point], occupancy[other]).OnlyEnforceIf(choices[point]);
                    }
                    model.AddEquality(x[worker].at(hour + 1), shed[point][0]).OnlyEnforceIf(choices[point]);
                    model.AddEquality(y[worker].at(hour + 1), shed[point][1]).OnlyEnforceIf(choices[point]);
                }
                for (int point = 0; point < 4; ++point) occupancy[point] += choices[point];
            }
        }
    }
    std::vector<Profile> selected(const sat::CpSolverResponse& response) const {
        std::vector<Profile> profiles;
        for (int worker = 0; worker < int(releases.size()); ++worker)
            profiles.push_back({{{int(sat::SolutionIntegerValue(response, x[worker].at(releases[worker]))),
                                  int(sat::SolutionIntegerValue(response, y[worker].at(releases[worker])))}}, releases[worker]});
        return profiles;
    }
};

struct RouteCheckpoints {
    sat::CpModelBuilder& model;
    std::string label;
    std::map<int, std::tuple<IntVar, IntVar, BoolVar>> points;
    std::map<std::pair<int, Point>, LinearExpr> distances;
    int nodes = 0;
    RouteCheckpoints(SpawnCheckpoints& checkpoints, IntVar worker, std::string name) : model(checkpoints.model), label(std::move(name)) {
        for (int hour : checkpoints.times) {
            auto x = integer(model, 0, 9, day_native::ready_screen::name(label + ".x", {hour}));
            auto y = integer(model, 0, 9, day_native::ready_screen::name(label + ".y", {hour}));
            auto alive = boolean(model, day_native::ready_screen::name(label + ".alive", {hour}));
            std::vector<LinearExpr> xs, ys;
            std::vector<int64_t> active;
            for (std::size_t w = 0; w < checkpoints.releases.size(); ++w) {
                xs.push_back(checkpoints.x[w].contains(hour) ? LinearExpr(checkpoints.x[w].at(hour)) : LinearExpr(0));
                ys.push_back(checkpoints.y[w].contains(hour) ? LinearExpr(checkpoints.y[w].at(hour)) : LinearExpr(0));
                active.push_back(checkpoints.releases[w] <= hour);
            }
            model.AddElement(worker, xs, x); model.AddElement(worker, ys, y); model.AddElement(worker, active, alive);
            points[hour] = {x, y, alive};
        }
    }
    void add_service(IntVar time, Point point, BoolVar active = {}) {
        const int index = nodes++;
        for (const auto& [hour, values] : points) {
            const auto [x, y, alive] = values;
            const auto key = std::pair(hour, point);
            if (!distances.contains(key))
                distances[key] = point_distance(model, x, y, point[0], point[1], label + ".distance[" + std::to_string(hour) + "," + point_name(point) + "]");
            const auto travel = distances.at(key);
            auto before = boolean(model, name(label + ".before", {index, hour}));
            model.AddLessThan(time, hour).OnlyEnforceIf(before);
            model.AddGreaterOrEqual(time, hour).OnlyEnforceIf(before.Not());
            std::vector<BoolVar> guards{alive};
            if (active.index() >= 0) guards.push_back(active);
            guards.push_back(before);
            model.AddLessOrEqual(time + 1 + travel, hour).OnlyEnforceIf(guards);
            guards.back() = before.Not();
            model.AddGreaterOrEqual(time, hour + travel).OnlyEnforceIf(guards);
        }
    }
};

} // namespace day_native::ready_screen
