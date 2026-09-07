#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <tuple>
#include <vector>

#include "generated_actions.hpp"

namespace league::public_unchanged::c68 {
namespace {

static_assert(generated::SOURCE_SHA256 ==
    "e5cbb6ed75e8582ed27dab18539f3b14e20f87d591181936c38cd1697ffbc248");

constexpr int PREEMPT_MAX_CLONE_DISTANCE = 6;
constexpr int PREEMPT_MIN_FUTURE_QUANTITY = 4;
constexpr int PREEMPT_START = 120;
constexpr int PREEMPT_STOP = 680;
constexpr int PREEMPT_MAX_BATCH = 30;
constexpr int WEED_REPLAY_STEPS = 8;
constexpr std::array<int, 4> PREMIUM{
    kag::STRAWBERRY, kag::MELON, kag::MILK, kag::WOOL};
constexpr std::array<int, 9> LIQUIDATION{
    kag::CARROT, kag::EGG, kag::FERTILIZER, kag::MELON, kag::MILK,
    kag::STRAWBERRY, kag::TOMATO, kag::WHEAT, kag::WOOL};

enum Shape : uint8_t { LINEAR, SQ, SQRT, LOG };

struct MarketParameters {
    double base;
    int equilibrium;
    double scale;
    Shape below_shape;
    double below_target;
    Shape above_shape;
    double above_target;
};

constexpr std::array<MarketParameters, kag::N_PRODUCTS> MARKET{{
    {25, 10000, 400, SQRT, .8, LOG, .2},
    {35, 10000, 450, LOG, .2, SQRT, .7},
    {60, 10000, 200, LINEAR, .4, SQRT, .6},
    {120, 10000, 100, SQRT, .7, LINEAR, 1.6},
    {250, 10000, 300, LOG, .2, SQ, 3.6},
    {50, 10000, 332, LINEAR, .4, LOG, .2},
    {160, 10000, 122, SQRT, .6, LINEAR, 1.6},
    {200, 10000, 105, LOG, .2, SQ, 3.2},
    {100, 10000, 200, LINEAR, .4, LINEAR, .4},
}};

int premium_index(int item) {
    for (int index = 0; index < static_cast<int>(PREMIUM.size()); ++index)
        if (PREMIUM[index] == item) return index;
    return -1;
}

double shape(Shape kind, double value) {
    value = std::max(0.0, value);
    switch (kind) {
        case LINEAR: return value;
        case SQ: return value * value;
        case SQRT: return std::sqrt(value);
        case LOG: return std::log1p(value);
    }
    std::abort();
}

int old_market_price(int item, int inventory) {
    const MarketParameters& parameters = MARKET[item];
    double price = 0;
    if (inventory < parameters.equilibrium) {
        const double amplitude = parameters.below_target * parameters.base /
            shape(parameters.below_shape, parameters.scale);
        price = parameters.base + amplitude * shape(
            parameters.below_shape, parameters.equilibrium - inventory);
    } else {
        const double amplitude = parameters.above_target * parameters.base /
            shape(parameters.above_shape, parameters.scale);
        price = parameters.base - amplitude * shape(
            parameters.above_shape, inventory - parameters.equilibrium);
    }
    return std::max(1, static_cast<int>(std::nearbyint(price)));
}

kag::UnitAction tape_unit(int step, int unit) {
    step = std::clamp(step, 0, static_cast<int>(generated::STEPS.size()) - 1);
    const generated::TapeStep& entry = generated::STEPS[step];
    if (unit < 0 || unit >= entry.n_units) return {};
    return generated::UNITS[entry.unit_offset + unit];
}

void tape_action(int step, int wanted_units, kag::Action& action) {
    action.clear();
    action.n_units = wanted_units;
    std::fill_n(action.units, wanted_units, kag::UnitAction{});
    const generated::TapeStep& entry = generated::STEPS[step];
    const int copied_units = std::min<int>(wanted_units, entry.n_units);
    std::copy_n(generated::UNITS.begin() + entry.unit_offset,
                copied_units, action.units);
    action.n_orders = entry.n_orders;
    if (action.n_orders > static_cast<int>(std::size(action.orders))) std::abort();
    std::copy_n(generated::ORDERS.begin() + entry.order_offset,
                action.n_orders, action.orders);
}

int planned_premium(int step, int item) {
    if (step < 0 || step >= static_cast<int>(generated::STEPS.size())) return 0;
    const generated::TapeStep& entry = generated::STEPS[step];
    int quantity = 0;
    for (int index = 0; index < entry.n_orders; ++index) {
        const kag::Order& order = generated::ORDERS[entry.order_offset + index];
        if (order.op == kag::M_SELL && order.item == item)
            quantity += std::max(0, order.n);
    }
    return quantity;
}

int town_drain(int step, const RaceState& race, int item) {
    int result = step % 24 == 0 ? 1 : 0;
    if (step % 4 != 0) return result;
    for (int index = 0; index < race.n_shops; ++index) {
        const int shop = race.shops[index];
        if (shop < kag::N_SHOPS &&
            (kag::SHOP_MASK[shop] & (uint16_t{1} << item)))
            result += kag::SHOP_MULT[shop];
    }
    return result;
}

std::array<int, 11> public_signature(const kag::agent::PublicFarm& farm) {
    std::array<int, 11> result{};
    // Hands and quadrant counts are compared separately by clone_distance.
    for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
        const kag::Tile& tile = farm.tiles[y][x];
        if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS) {
            ++result[tile.what];
        } else if (tile.has_animal) {
            if (tile.what == kag::COW) ++result[5];
            else if (tile.what == kag::SHEEP) ++result[6];
            else if (tile.what == kag::GOOSE) ++result[7];
        } else if (tile.kind == kag::T_PASTURE) {
            ++result[8];
        } else if (tile.kind == kag::T_COOP) {
            ++result[9];
        } else if (tile.kind == kag::T_WEED) {
            ++result[10];
        }
    }
    return result;
}

int clone_distance(const kag::agent::AgentObservation& observation) {
    const auto left = public_signature(observation.farms[0]);
    const auto right = public_signature(observation.farms[1]);
    int result = std::abs(observation.farms[0].n_units -
                          observation.farms[1].n_units);
    result += 3 * std::abs(observation.farms[0].n_quadrants -
                           observation.farms[1].n_quadrants);
    for (int index = 0; index < static_cast<int>(left.size()); ++index)
        result += std::abs(left[index] - right[index]);
    return result;
}

void reset_race(RaceState& race) {
    race = {};
    race.last_step = -1;
    race.horizon = 4;
}

void observe_opponent_market(const kag::agent::AgentObservation& observation,
                             RaceState& race, int step) {
    if (step == 0 || step < race.last_step) reset_race(race);
    if (race.has_inventory && race.last_step == step - 1 &&
        clone_distance(observation) <= PREEMPT_MAX_CLONE_DISTANCE) {
        for (const int item : PREMIUM) {
            const int inferred =
                observation.market.inventory[item] - race.inventory[item] +
                town_drain(race.last_step, race, item) - race.own_sells[item] -
                planned_premium(race.last_step, item);
            if (inferred < PREEMPT_MIN_FUTURE_QUANTITY) continue;
            ++race.events;
            for (int horizon = 1; horizon <= 6; ++horizon) {
                const int expected = planned_premium(race.last_step + horizon, item);
                if (expected > 0) {
                    const double similarity = static_cast<double>(
                        std::min(inferred, expected)) / std::max(inferred, expected);
                    race.scores[horizon] += 1.0 + similarity;
                } else {
                    race.scores[horizon] -= .15;
                }
            }
        }
        if (race.events >= 2) {
            int best = 1;
            for (int horizon = 2; horizon <= 6; ++horizon)
                if (race.scores[horizon] > race.scores[best]) best = horizon;
            race.horizon = std::min(7, std::max(2, best + 1));
        }
    }
    race.last_step = step;
    race.has_inventory = true;
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        race.inventory[item] = observation.market.inventory[item];
    race.n_shops = observation.n_shops;
    std::copy_n(observation.shops, race.n_shops, race.shops.begin());
}

void weed_repair(const kag::agent::AgentObservation& observation,
                 SeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.weed_last_step) {
        state.weeds.fill({});
        for (WeedTransaction& transaction : state.weeds)
            transaction.start = -1;
    }
    state.weed_last_step = step;
    for (int unit = 0; unit < kag::MAX_UNITS; ++unit)
        if (unit >= action.n_units) state.weeds[unit] = {};
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = state.weeds[unit];
        if (!transaction.active) continue;
        const int age = step - transaction.start;
        if (age == 1) {
            action.units[unit] = transaction.intended;
        } else if (age >= 2 && age <= 1 + WEED_REPLAY_STEPS) {
            action.units[unit] = tape_unit(step - 1, unit);
        } else {
            transaction = {};
        }
    }
    const auto& farm = observation.self();
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = state.weeds[unit];
        const uint8_t op = action.units[unit].op;
        if (transaction.active ||
            (op != kag::OP_BUILD_PASTURE && op != kag::OP_PLANT))
            continue;
        const int x = farm.pos_x[unit];
        const int y = farm.pos_y[unit];
        if (farm.tiles[y][x].kind != kag::T_WEED) continue;
        transaction.active = true;
        transaction.start = step;
        transaction.intended = action.units[unit];
        action.units[unit] = {kag::OP_DIG, kag::N_ITEMS, 1};
    }
}

void reset_shift(SeatState& state, int step) {
    state.shift_last_step = step;
    for (auto& row : state.debts) row.fill(0);
}

void repay_shift(SeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.shift_last_step) reset_shift(state, step);
    state.shift_last_step = step;
    if (step < 0 || step >= static_cast<int>(state.debts.size())) return;
    std::array<int, 4> due = state.debts[step];
    state.debts[step].fill(0);
    if (std::all_of(due.begin(), due.end(), [](int value) { return value == 0; }))
        return;
    int kept = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        kag::Order order = action.orders[index];
        const int premium = premium_index(order.item);
        if (order.op == kag::M_SELL && premium >= 0 && due[premium] > 0) {
            const int reduction = std::min(std::max(0, order.n), due[premium]);
            order.n -= reduction;
            due[premium] -= reduction;
            if (order.n <= 0) continue;
        }
        action.orders[kept++] = order;
    }
    action.n_orders = kept;
}

std::array<int, kag::N_ITEMS> projected_shed(
    const kag::agent::AgentObservation& observation, const kag::Action& action) {
    std::array<int, kag::N_ITEMS> projected{};
    for (int item = 0; item < kag::N_ITEMS; ++item)
        projected[item] = std::max<int>(0, observation.own.shed[item]);
    int total = 0;
    for (const int quantity : projected) total += quantity;
    const auto& farm = observation.self();
    for (int unit = 0; unit < action.n_units; ++unit) {
        const int x = farm.pos_x[unit];
        const int y = farm.pos_y[unit];
        if (!((x == 4 || x == 5) && (y == 4 || y == 5))) continue;
        const kag::UnitAction& unit_action = action.units[unit];
        if (unit_action.op == kag::OP_DROP) {
            for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key) {
                const int item = observation.own.inv_keys[unit][key];
                const int amount = std::min<int>(
                    std::max(0, 100 - total),
                    std::max<int>(0, observation.own.inv[unit][item]));
                projected[item] += amount;
                total += amount;
            }
        } else if (unit_action.op == kag::OP_PLACE &&
                   unit_action.arg < kag::N_ITEMS) {
            const int item = unit_action.arg;
            const kag::Tile& tile = farm.tiles[y][x];
            const bool matching_empty_structure =
                kag::is_animal(item) && !tile.has_animal &&
                ((item == kag::GOOSE && tile.kind == kag::T_COOP) ||
                 ((item == kag::COW || item == kag::SHEEP) &&
                  tile.kind == kag::T_PASTURE));
            if (matching_empty_structure) continue;
            const int requested = std::max(0, unit_action.n);
            const int amount = std::min({
                requested, std::max<int>(0, observation.own.inv[unit][item]),
                std::max(0, 100 - total)});
            projected[item] += amount;
            total += amount;
        }
    }
    return projected;
}

double demand_per_day(const kag::agent::AgentObservation& observation, int item) {
    double demand = 0;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (shop < kag::N_SHOPS &&
            (kag::SHOP_MASK[shop] & (uint16_t{1} << item)))
            demand += 6.0 * kag::SHOP_MULT[shop];
    }
    if (item != kag::FERTILIZER) demand += 1.0;
    return demand;
}

double order_score(const kag::agent::AgentObservation& observation,
                   const kag::Order& order) {
    if (order.op != kag::M_SELL || order.item >= kag::N_PRODUCTS)
        return -std::numeric_limits<double>::infinity();
    const int quantity = std::max(0, order.n);
    const int item = order.item;
    const int current_inventory = observation.market.inventory[item];
    const double current_quote = observation.market.prices[item];
    const double later_quote = old_market_price(item, current_inventory + quantity);
    double score = quantity * std::max(0.0, current_quote - later_quote);
    if (score <= 0) return score;
    const double demand = std::max(.25, demand_per_day(observation, item));
    const double excess = std::max(
        0.0, static_cast<double>(current_inventory + quantity - 10000));
    const double urgency = std::min(1.0, excess / demand / 10.0);
    return score * (1.0 + .25 * urgency);
}

void rank_sell_slots(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    struct Row { double score; int index; kag::Order order; };
    std::vector<Row> rows;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item < kag::N_PRODUCTS)
            rows.push_back({order_score(observation, action.orders[index]),
                            index, action.orders[index]});
    if (rows.size() < 2) return;
    std::sort(rows.begin(), rows.end(), [](const Row& left, const Row& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.index < right.index;
    });
    int ranked = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item < kag::N_PRODUCTS)
            action.orders[index] = rows[ranked++].order;
}

void preempt_shift(const kag::agent::AgentObservation& observation,
                   SeatState& state, int step, kag::Action& action) {
    if (step < PREEMPT_START || step >= PREEMPT_STOP ||
        clone_distance(observation) > PREEMPT_MAX_CLONE_DISTANCE)
        return;
    const int horizon = state.race.horizon;
    std::array<int, 4> future{};
    for (int index = 0; index < static_cast<int>(PREMIUM.size()); ++index)
        future[index] = planned_premium(step + horizon, PREMIUM[index]);
    if (std::all_of(future.begin(), future.end(),
                    [](int value) { return value == 0; }) || action.n_orders >= 10)
        return;
    std::array<int, kag::N_ITEMS> remaining = projected_shed(observation, action);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_ITEMS)
            remaining[order.item] = std::max(
                0, remaining[order.item] - std::max(0, order.n));
    }
    std::array<int, 4> shifted{};
    for (int index = 0; index < static_cast<int>(PREMIUM.size()); ++index) {
        const int item = PREMIUM[index];
        const int future_quantity = std::max(0, future[index]);
        if (future_quantity < PREEMPT_MIN_FUTURE_QUANTITY) continue;
        const int target = std::min({
            std::max(0, remaining[item]), future_quantity, PREEMPT_MAX_BATCH,
            std::max(1, 2 * future_quantity)});
        if (target <= 0 || action.n_orders >= 10) continue;
        action.orders[action.n_orders++] = {
            kag::M_SELL, static_cast<uint8_t>(item), target};
        remaining[item] = std::max(0, remaining[item] - target);
        shifted[index] = target;
    }
    const int due_step = step + horizon;
    if (due_step < static_cast<int>(state.debts.size()))
        for (int index = 0; index < static_cast<int>(shifted.size()); ++index)
            state.debts[due_step][index] += shifted[index];
}

void terminal_liquidation(const kag::agent::AgentObservation& observation,
                          int step, kag::Action& action) {
    if (step < 716) return;
    std::array<int, kag::N_PRODUCTS> planned{};
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
            planned[order.item] += std::max(0, order.n);
    }
    for (const int item : LIQUIDATION) {
        const int available = std::max<int>(0, observation.own.shed[item]);
        const int extra = step >= 718
            ? available : std::max(0, available - planned[item]);
        if (extra > 0 && action.n_orders < 10)
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item), extra};
    }
}

void record_own_sells(RaceState& race, const kag::Action& action) {
    race.own_sells.fill(0);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && premium_index(order.item) >= 0)
            race.own_sells[order.item] += std::max(0, order.n);
    }
}

}

kag::agent::AgentInfo Policy::info() {
    return {"public-c68-thunder-adaptive-exact-port"};
}

void Policy::reset(const kag::agent::AgentInit&) {
    seats_ = {};
    for (SeatState& state : seats_) {
        state.weed_last_step = -1;
        state.shift_last_step = -1;
        reset_race(state.race);
    }
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget&, kag::Action& action) {
    if (observation.player > 1 || observation.self().n_units > kag::MAX_UNITS)
        std::abort();
    const int step = std::clamp(
        observation.step, 0, static_cast<int>(generated::STEPS.size()) - 1);
    SeatState& state = seats_[observation.player];
    observe_opponent_market(observation, state.race, step);
    tape_action(step, observation.self().n_units, action);
    weed_repair(observation, state, step, action);
    if (preempt_enabled_) repay_shift(state, step, action);
    rank_sell_slots(observation, action);
    if (preempt_enabled_) preempt_shift(observation, state, step, action);
    terminal_liquidation(observation, step, action);
    record_own_sells(state.race, action);
    action.finalize();
}

}
