#pragma once
#include <day_solver/io.hpp>
#include "actions.hpp"
#include "fast_day_solver_estimator/estimator.hpp"
#include "terminal_deadlines.hpp"
#include <filesystem>
#include <numeric>
#include <unordered_map>

namespace placement {
namespace fs = std::filesystem;
using day_solver::DayProblem;
using Schedule = labor::offline::Schedule;
using Layout = std::array<uint8_t, 100>;

inline Layout identity() { Layout result; std::iota(result.begin(), result.end(), 0); return result; }
inline int quadrant(int cell) { return kag::quadrant_of(cell % 10, cell / 10, 10); }
inline std::string day_name(int day) { return (day < 10 ? "0" : "") + std::to_string(day); }

struct Day {
    DayProblem problem;
    Schedule physical, executable;
    labor::PlanningMenu menu;
};

inline labor::PlanningMenu legal_menu(const Day& day, int hours) {
    std::vector<std::pair<int, int>> slots;
    for (int h = 0; h < hours - 1 && slots.size() < 39; ++h)
        for (int s = 0; s < 10 && slots.size() < 39; ++s) {
            const auto& a = day.executable[h];
            if (s >= a.n_orders || a.orders[s].op == kag::M_NONE || a.orders[s].op == kag::M_HIRE)
                slots.emplace_back(h, s);
        }
    return labor::planning_menu(day.problem, hours, slots);
}

struct Course {
    std::array<Day, 30> days;
    std::array<std::array<int, 100>, 30> tasks{};
    std::array<double, 100> frequency{}, actions{}, shadow{}, input_output{};
    std::array<bool, 100> fixed{};
    explicit Course(const fs::path& path) {
        for (int d = 0; d < 30; ++d) {
            auto& day = days[d];
            const auto folder = path / day_name(d);
            day.problem = day_solver::load_problem_json(folder / "problem.json");
            day.physical = labor::offline::read_actions((folder / "physical.actions.txt").string());
            day.executable = labor::offline::read_actions((folder / "executable.actions.txt").string());
            if (day.problem.start.managed_tiles.size() != 100) throw std::runtime_error("course needs all board cells");
            for (int i = 0; i < 100; ++i) {
                const auto& tile = day.problem.start.managed_tiles[i];
                if (tile.y * 10 + tile.x != i) throw std::runtime_error("source tile IDs must be board cells");
                if (d == 0 && tile.state.kind != day_solver::ManagedTileKind::EMPTY &&
                    tile.state.kind != day_solver::ManagedTileKind::LOCKED) fixed[i] = true;
            }
            day_scheduler::prepare_problem(day.problem);
            if (d == 29) labor::offline::require_terminal_work(day.problem);
            day.menu = legal_menu(day, d == 29 ? 23 : 24);
            const auto replay = day_solver::replay_schedule(day.problem, day.physical);
            if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied ||
                !replay.invariants_satisfied || !replay.errors.empty()) throw std::runtime_error("invalid source day " + std::to_string(d));
            const double price = std::max<int64_t>(1, labor::hire_cost(day.problem.worker_count) -
                                                     labor::hire_cost(std::max(1, int(day.problem.worker_count) - 1)));
            for (const auto& work : day.problem.tile_work) {
                const int cell = work.tile;
                tasks[d][cell] = work.actions.size();
                frequency[cell] += !work.actions.empty(); actions[cell] += work.actions.size();
                shadow[cell] += price * work.actions.size();
                for (const auto& action : work.actions)
                    input_output[cell] += action.op == kag::OP_FEED || action.op == kag::OP_FERTILIZE ||
                                          action.op == kag::OP_PLACE || action.op == kag::OP_HARVEST;
            }
        }
    }
    bool legal(const Layout& layout) const {
        std::array<bool, 100> seen{};
        for (int i = 0; i < 100; ++i) {
            const int target = layout[i];
            if (target >= 100 || seen[target] || quadrant(i) != quadrant(target) || (fixed[i] && target != i)) return false;
            seen[target] = true;
        }
        return true;
    }
    DayProblem problem(int d, const Layout& layout) const {
        auto p = days[d].problem;
        for (int i = 0; i < 100; ++i) {
            p.start.managed_tiles[i].x = layout[i] % 10;
            p.start.managed_tiles[i].y = layout[i] / 10;
        }
        // Only coordinates changed; all derived non-geometric commitments stay valid.
        return p;
    }
};

inline DayProblem workforce(DayProblem p, const labor::PlanningMenu& menu, int workers, int day) {
    if (workers < 1 || workers > menu.size + 1) throw std::runtime_error("illegal workforce");
    std::erase_if(p.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
    p.worker_count = workers;
    for (int i = 0; i < workers - 1; ++i) {
        day_solver::MarketEvent event;
        event.hour = menu.hires.hours[i]; event.order_index = menu.hires.slots[i];
        event.market_op = kag::M_HIRE; event.quantity = 1;
        p.market_plan.push_back(event);
    }
    std::sort(p.market_plan.begin(), p.market_plan.end(), [](const auto& a, const auto& b) {
        return std::pair{a.hour, a.order_index} < std::pair{b.hour, b.order_index};
    });
    day_scheduler::prepare_problem(p);
    if (day == 29) labor::offline::require_terminal_work(p);
    return p;
}

inline Schedule executable(const Day& source, const DayProblem& problem, Schedule physical) {
    for (int h = 0; h < 24; ++h) {
        auto& a = physical[h];
        a.n_orders = source.executable[h].n_orders;
        std::copy_n(source.executable[h].orders, 10, a.orders);
        for (auto& order : a.orders) if (order.op == kag::M_HIRE) order = {};
    }
    for (const auto& event : problem.market_plan) if (event.market_op == kag::M_HIRE) {
        auto& a = physical[event.hour];
        if (a.orders[event.order_index].op != kag::M_NONE) throw std::runtime_error("hire overwrites fixed order");
        a.orders[event.order_index] = {kag::M_HIRE, 0, event.quantity};
        a.n_orders = std::max(a.n_orders, int(event.order_index) + 1);
    }
    for (auto& a : physical) a.finalize();
    return physical;
}
}
