#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace kag::agents::cold_capacity_wheat {
namespace {

constexpr std::array<std::array<int, 2>, 8> PLOTS{{
    {{3, 4}}, {{2, 4}}, {{1, 4}}, {{0, 4}},
    {{4, 3}}, {{3, 3}}, {{2, 3}}, {{1, 3}},
}};

UnitAction move_toward(int x, int y, int target_x, int target_y) {
    if (x < target_x) return {OP_EAST, 0, 1};
    if (x > target_x) return {OP_WEST, 0, 1};
    if (y < target_y) return {OP_SOUTH, 0, 1};
    if (y > target_y) return {OP_NORTH, 0, 1};
    return {};
}

}

agent::AgentInfo Agent::info() {
    return {"cold_capacity_wheat", agent::API_FORMAT_VERSION};
}

void Agent::reset(const agent::AgentInit& init) {
    config_ = init.config;
    player_ = init.player;
}

void Agent::act(const agent::AgentObservation& observation,
                const agent::DecisionBudget&,
                Action& action) {
    if (observation.player != player_) std::abort();

    action.clear();
    const auto& farm = observation.self();
    action.n_units = farm.n_units;
    for (int unit = 0; unit < action.n_units; ++unit)
        action.units[unit] = UnitAction{};

    int seed_remaining = observation.own.seeds[WHEAT];
    int shed_room = config_.shed_capacity - observation.own.shed_total;
    int planned_place = 0;
    int planned_harvest = 0;
    int carried_wheat = 0;
    for (int unit = 0; unit < farm.n_units; ++unit)
        carried_wheat += observation.own.inv[unit][WHEAT];

    const int controlled_units = std::min<int>(farm.n_units, PLOTS.size());
    for (int unit = 0; unit < controlled_units; ++unit) {
        const int x = farm.pos_x[unit];
        const int y = farm.pos_y[unit];
        const int cargo = observation.own.inv[unit][WHEAT];
        if (cargo > 0) {
            if (is_shed_adjacent(x, y, config_.board_size) && shed_room > 0) {
                const int quantity = std::min(cargo, shed_room);
                action.units[unit] = {OP_PLACE, WHEAT, quantity};
                shed_room -= quantity;
                planned_place += quantity;
            } else {
                action.units[unit] = move_toward(x, y, 4, 4);
            }
            continue;
        }

        const int target_x = PLOTS[unit][0];
        const int target_y = PLOTS[unit][1];
        if (x != target_x || y != target_y) {
            action.units[unit] = move_toward(x, y, target_x, target_y);
            continue;
        }

        const Tile& tile = farm.tiles[y][x];
        if (tile.kind == T_WEED) {
            action.units[unit] = {OP_DIG, 0, 1};
        } else if (tile.kind == T_EMPTY && seed_remaining > 0 &&
                   observation.hour <= 20) {
            action.units[unit] = {OP_PLANT, WHEAT, 1};
            --seed_remaining;
        } else if (tile.kind == T_PLANT && tile.what == WHEAT) {
            const int age = observation.day - tile.planted_day;
            if (age > 4 && tile.yield_units > 0) {
                action.units[unit] = {OP_HARVEST, 0, 1};
                planned_harvest += tile.yield_units;
            } else if (!tile.watered_today) {
                action.units[unit] = {OP_WATER, 0, 1};
            } else if (age >= 4 && tile.yield_units > 0) {
                action.units[unit] = {OP_HARVEST, 0, 1};
                planned_harvest += tile.yield_units;
            }
        }
    }

    double budget = farm.money;
    if (observation.hour == 0) {
        int open_plots = 0;
        for (const auto& plot : PLOTS) {
            const Tile& tile = farm.tiles[plot[1]][plot[0]];
            open_plots += tile.kind == T_EMPTY || tile.kind == T_WEED;
        }
        const int desired_seeds = open_plots + 2;
        int quantity = std::max(0, desired_seeds - observation.own.seeds[WHEAT]);
        quantity = std::min(quantity,
                            static_cast<int>(budget / CROPS[WHEAT].seed));
        if (quantity > 0) {
            action.orders[action.n_orders++] = {M_BUY_SEED, WHEAT, quantity};
            budget -= quantity * CROPS[WHEAT].seed;
        }

        const int desired_hands = static_cast<int>(PLOTS.size()) - 1;
        for (int hand = farm.hires_today;
             hand < desired_hands && action.n_orders < config_.max_orders - 1;
             ++hand) {
            const int cost = config_.hire_mult * fib(hand);
            if (budget < cost) break;
            action.orders[action.n_orders++] = {M_HIRE, 0, 1};
            budget -= cost;
        }
    }

    const int total_wheat = observation.own.shed[WHEAT] + carried_wheat +
                            planned_harvest;
    const int overflow = std::max(0, total_wheat - 92);
    const int sellable = observation.own.shed[WHEAT] + planned_place;
    const int sell_quantity = std::min(overflow, sellable);
    if (sell_quantity > 0 && action.n_orders < config_.max_orders)
        action.orders[action.n_orders++] = {M_SELL, WHEAT, sell_quantity};

    action.finalize();
}

}
