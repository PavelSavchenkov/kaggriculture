#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <tuple>

#include "generated_data.hpp"

namespace league::public_unchanged::indar {
namespace {

static_assert(generated::SOURCE_SHA256 ==
    "d39dba50793d9777c990347443bf0c481c78adaea86055f6f6b0600dcfcd9f2e");
static_assert(generated::LOW_SHA256 ==
    "8ef00c36210774c885c5ea69c356b771ea7b43e793a514c4d25a651af36f1c5d");
static_assert(generated::HIGH_SHA256 ==
    "9c10b825efdb28e1b0feef975804d5453139752858b746c2ceb4077b5023d407");
static_assert(generated::R5_SHA256 ==
    "563973193a03a1c80c1b4b01ddc03a02455b8189cd0cb73ad4bf496328b64a7b");
static_assert(generated::MD_SHA256 ==
    "4e8feb3e728023e69f7ed2a8c64e5e00438fe3606f4c85fe2919115c83df6577");

constexpr int EXPERT_LOW = 0;
constexpr int EXPERT_HIGH = 1;
constexpr int WEED_REPLAY_STEPS = 8;
constexpr std::array<int, 4> PREMIUM{
    kag::MELON, kag::MILK, kag::STRAWBERRY, kag::WOOL};
constexpr std::array<int, 9> LIQUIDATION{
    kag::CARROT, kag::EGG, kag::FERTILIZER, kag::MELON, kag::MILK,
    kag::STRAWBERRY, kag::TOMATO, kag::WHEAT, kag::WOOL};
constexpr std::array<int, 9> ROOM_PRIORITY{
    kag::WOOL, kag::MILK, kag::EGG, kag::MELON, kag::STRAWBERRY,
    kag::TOMATO, kag::CARROT, kag::FERTILIZER, kag::WHEAT};

enum Shape : int { LINEAR, SQ, SQRT, LOG };
struct MarketParam {
    double base;
    int equilibrium;
    double scale;
    Shape below_shape;
    double below_target;
    Shape above_shape;
    double above_target;
};
constexpr std::array<MarketParam, kag::N_PRODUCTS> MARKET_PARAMS{{
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

double shape(Shape name, double value) {
    value = std::max(0.0, value);
    if (name == LINEAR) return value;
    if (name == SQ) return value * value;
    if (name == SQRT) return std::sqrt(value);
    return std::log1p(value);
}

int reconstructed_market_price(int item, int inventory) {
    const MarketParam& param = MARKET_PARAMS[item];
    double price = 0;
    if (inventory < param.equilibrium) {
        const double amplitude = param.below_target * param.base /
            shape(param.below_shape, param.scale);
        price = param.base + amplitude *
            shape(param.below_shape, param.equilibrium - inventory);
    } else {
        const double amplitude = param.above_target * param.base /
            shape(param.above_shape, param.scale);
        price = param.base - amplitude *
            shape(param.above_shape, inventory - param.equilibrium);
    }
    return std::max(1, static_cast<int>(std::nearbyint(price)));
}

const std::array<generated::TapeStep, 719>& tape(int expert) {
    return expert == EXPERT_HIGH ? generated::HIGH_STEPS : generated::LOW_STEPS;
}

const generated::TapeStep& tape_step(int expert, int step) {
    const auto& selected = tape(expert);
    return selected[std::clamp(step, 0, static_cast<int>(selected.size()) - 1)];
}

kag::UnitAction tape_unit(int expert, int step, int unit) {
    const generated::TapeStep& entry = tape_step(expert, step);
    if (unit < 0 || unit >= entry.n_units) return {};
    return generated::UNITS[entry.unit_offset + unit];
}

void tape_action(int expert, int step, int wanted_units, kag::Action& action) {
    action.clear();
    action.n_units = wanted_units;
    std::fill_n(action.units, wanted_units, kag::UnitAction{});
    const generated::TapeStep& entry = tape_step(expert, step);
    const int copied_units = std::min<int>(wanted_units, entry.n_units);
    std::copy_n(generated::UNITS.begin() + entry.unit_offset,
                copied_units, action.units);
    action.n_orders = entry.n_orders;
    if (action.n_orders > 10) std::abort();
    std::copy_n(generated::ORDERS.begin() + entry.order_offset,
                action.n_orders, action.orders);
}

int select_expert(const kag::agent::AgentObservation& observation,
                  SeatState& state) {
    const int step = observation.step;
    if (step == 0 || step < state.selector_last_step) {
        state.selector_last_step = step;
        state.selector_expert = -1;
        state.selector_n_shops = 0;
    }
    state.selector_last_step = step;
    if (step <= 168) {
        state.selector_n_shops = observation.n_shops;
        std::copy_n(observation.shops, observation.n_shops,
                    state.selector_shops.begin());
    }
    if (state.selector_expert < 0 && step >= 168) {
        bool yarn = false;
        for (int index = 0; index < state.selector_n_shops; ++index)
            yarn |= state.selector_shops[index] == kag::SHOP_YARN_STORE;
        const bool dominated = state.selector_n_shops >= 2 &&
            state.selector_shops[0] == kag::SHOP_ICE_CREAM_SHOP &&
            state.selector_shops[1] == kag::SHOP_YARN_STORE;
        state.selector_expert = yarn && !dominated ? EXPERT_HIGH : EXPERT_LOW;
    }
    return state.selector_expert < 0 ? EXPERT_LOW : state.selector_expert;
}

void reset_weeds(SeatState& state, int step) {
    state.weed_last_step = step;
    state.weeds.fill({});
    for (WeedTransaction& transaction : state.weeds) transaction.start = -1;
}

void weed_repair(const kag::agent::AgentObservation& observation,
                 int expert, int step, SeatState& state, kag::Action& action) {
    if (step == 0 || step < state.weed_last_step) reset_weeds(state, step);
    state.weed_last_step = step;
    for (int unit = action.n_units; unit < kag::MAX_UNITS; ++unit)
        state.weeds[unit] = {};
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = state.weeds[unit];
        if (!transaction.active) continue;
        const int age = step - transaction.start;
        if (age == 1) {
            action.units[unit] = transaction.intended;
        } else if (age >= 2 && age <= 1 + WEED_REPLAY_STEPS) {
            action.units[unit] = tape_unit(expert, step - 1, unit);
        } else {
            transaction = {};
        }
    }
    const auto& farm = observation.self();
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = state.weeds[unit];
        const uint8_t op = action.units[unit].op;
        if (transaction.active ||
            (op != kag::OP_BUILD_PASTURE && op != kag::OP_PLANT)) continue;
        const int x = farm.pos_x[unit];
        const int y = farm.pos_y[unit];
        if (farm.tiles[y][x].kind != kag::T_WEED) continue;
        transaction.active = true;
        transaction.start = step;
        transaction.intended = action.units[unit];
        action.units[unit] = {kag::OP_DIG, kag::N_ITEMS, 1};
    }
}

int count_animal(const kag::agent::PublicFarm& farm, int animal) {
    int result = 0;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            result += farm.tiles[y][x].has_animal &&
                      farm.tiles[y][x].what == animal;
    return result;
}

int pickup_reserve(const kag::Action& action, int item) {
    int result = 0;
    for (int unit = 0; unit < action.n_units; ++unit)
        if (action.units[unit].op == kag::OP_PICKUP &&
            action.units[unit].arg == item)
            result += std::max(0, action.units[unit].n);
    return result;
}

int existing_sell(const kag::Action& action, int item) {
    int result = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item == item)
            result += std::max(0, action.orders[index].n);
    return result;
}

int planned_sell(const auto& markets, int step, int item) {
    if (step < 0 || step >= static_cast<int>(markets.size())) return 0;
    const generated::MarketStep& entry = markets[step];
    int result = 0;
    for (int index = 0; index < entry.n_orders; ++index) {
        const kag::Order& order = generated::ORDERS[entry.order_offset + index];
        if (order.op == kag::M_SELL && order.item == item)
            result += std::max(0, order.n);
    }
    return result;
}

int town_demand_at(const kag::agent::AgentObservation& observation,
                   int item, int step) {
    int demand = item != kag::FERTILIZER && step % 24 == 0 ? 1 : 0;
    if (step % 4 != 0) return demand;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (kag::SHOP_MASK[shop] & (uint16_t{1} << item))
            demand += kag::SHOP_MULT[shop];
    }
    return demand;
}

void add_or_merge_sell(kag::Action& action, int item, int quantity) {
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item == item) {
            action.orders[index].n = std::max(0, action.orders[index].n) + quantity;
            return;
        }
    if (action.n_orders < 10)
        action.orders[action.n_orders++] = {
            kag::M_SELL, static_cast<uint8_t>(item), quantity};
}

void r5_counter(const kag::agent::AgentObservation& observation,
                SeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.r5_last_step) {
        state.r5_last_step = step;
        state.r5_target = false;
    }
    state.r5_last_step = step;
    if (!state.r5_target && step >= 24) {
        const int cows = count_animal(observation.opponent(), kag::COW);
        const int sheep = count_animal(observation.opponent(), kag::SHEEP);
        if (sheep >= 4 && cows <= 3) state.r5_target = true;
    }
    if (!state.r5_target || step + 2 >=
        static_cast<int>(generated::R5_STEPS.size())) return;
    for (const int item : PREMIUM) {
        const int planned = planned_sell(generated::R5_STEPS, step + 2, item);
        if (planned <= 0 || town_demand_at(observation, item, step) > 0 ||
            town_demand_at(observation, item, step + 1) > 0) continue;
        const int available = std::max<int>(
            0, observation.own.shed[item] - existing_sell(action, item) -
               pickup_reserve(action, item));
        const int quantity = std::min(available, planned);
        if (quantity > 0) add_or_merge_sell(action, item, quantity);
    }
    action.n_orders = std::min(action.n_orders, 10);
}

void md_counter(const kag::agent::AgentObservation& observation,
                SeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.md_last_step) {
        state.md_last_step = step;
        state.md_target = false;
    }
    state.md_last_step = step;
    if (!state.md_target && step >= 160) {
        const int cows = count_animal(observation.opponent(), kag::COW);
        const int sheep = count_animal(observation.opponent(), kag::SHEEP);
        if ((observation.opponent().n_quadrants >= 2 && cows >= 4 && sheep <= 2) ||
            cows >= 9) state.md_target = true;
    }
    if (!state.md_target || step + 1 >=
        static_cast<int>(generated::MD_STEPS.size())) return;
    for (const int item : PREMIUM) {
        const int target = planned_sell(generated::MD_STEPS, step + 1, item);
        if (target <= 0) continue;
        const int available = std::max<int>(
            0, observation.own.shed[item] - existing_sell(action, item) -
               pickup_reserve(action, item));
        const int quantity = std::min(available, 2 * target);
        if (quantity > 0) add_or_merge_sell(action, item, quantity);
    }
    action.n_orders = std::min(action.n_orders, 10);
}

double demand_per_day(const kag::agent::AgentObservation& observation,
                      int item) {
    double demand = 0;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (kag::SHOP_MASK[shop] & (uint16_t{1} << item))
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
    const int inventory = observation.market.inventory[item];
    const double current = observation.market.prices[item];
    const double later = reconstructed_market_price(item, inventory + quantity);
    double score = quantity * std::max(0.0, current - later);
    if (score <= 0) return score;
    const double demand = std::max(.25, demand_per_day(observation, item));
    const double excess = std::max(
        0.0, static_cast<double>(inventory + quantity - 10000));
    const double urgency = std::min(1.0, excess / demand / 10.0);
    return score * (1.0 + .25 * urgency);
}

void rank_sell_slots(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    struct Row { double score; int index; kag::Order order; };
    std::array<Row, 10> rows{};
    int n_rows = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item < kag::N_PRODUCTS)
            rows[n_rows++] = {order_score(observation, action.orders[index]),
                              index, action.orders[index]};
    if (n_rows < 2) return;
    std::sort(rows.begin(), rows.begin() + n_rows,
              [](const Row& left, const Row& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.index < right.index;
    });
    int ranked = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item < kag::N_PRODUCTS)
            action.orders[index] = rows[ranked++].order;
}

kag::UnitAction move_toward(int x, int y, int tx, int ty) {
    if (x < tx) return {kag::OP_EAST, kag::N_ITEMS, 1};
    if (x > tx) return {kag::OP_WEST, kag::N_ITEMS, 1};
    if (y < ty) return {kag::OP_SOUTH, kag::N_ITEMS, 1};
    if (y > ty) return {kag::OP_NORTH, kag::N_ITEMS, 1};
    return {};
}

constexpr std::array<std::array<int, 2>, 4> SHED_SET_ORDER{{
    {4, 4}, {5, 4}, {5, 5}, {4, 5},
}};

std::array<int, 2> nearest_shed(int x, int y) {
    std::array<int, 2> result = SHED_SET_ORDER[0];
    int best = std::abs(x - result[0]) + std::abs(y - result[1]);
    for (int index = 1; index < 4; ++index) {
        const int distance = std::abs(x - SHED_SET_ORDER[index][0]) +
                             std::abs(y - SHED_SET_ORDER[index][1]);
        if (distance < best) {
            best = distance;
            result = SHED_SET_ORDER[index];
        }
    }
    return result;
}

int inventory_total(const kag::agent::AgentObservation& observation) {
    int total = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key)
            total += std::max<int>(0, observation.own.inv[unit]
                [observation.own.inv_keys[unit][key]]);
    return total;
}

void room_evac(const kag::agent::AgentObservation& observation,
               SeatState& state, int step, kag::Action& action) {
    if (step < 648) return;
    if (step == 0 || step < state.room_last_step ||
        observation.day != state.room_day) {
        state.room_last_step = step;
        state.room_day = observation.day;
        state.room_actor = -1;
    }
    state.room_last_step = step;
    if (observation.hour < 21) return;
    const int total = inventory_total(observation);
    if (observation.hour == 21 && state.room_actor < 0 && total > 100) {
        bool found = false;
        std::tuple<int, int, int, int, int> best{};
        for (int unit = 0; unit < action.n_units; ++unit) {
            int saleable = 0;
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                saleable += std::max<int>(0, observation.own.inv[unit][item]);
            if (saleable <= 0 || action.units[unit].op != kag::OP_PASS) continue;
            const int x = observation.self().pos_x[unit];
            const int y = observation.self().pos_y[unit];
            const auto target = nearest_shed(x, y);
            const int distance = std::abs(x - target[0]) +
                                 std::abs(y - target[1]);
            if (distance > 2) continue;
            const auto candidate = std::make_tuple(
                distance, -saleable, unit, target[0], target[1]);
            if (!found || candidate < best) {
                found = true;
                best = candidate;
            }
        }
        if (found) {
            state.room_actor = std::get<2>(best);
            state.room_target_x = std::get<3>(best);
            state.room_target_y = std::get<4>(best);
        }
    }
    if (state.room_actor < 0) return;
    const int actor = state.room_actor;
    if (actor >= action.n_units) {
        state.room_actor = -1;
        return;
    }
    const int x = observation.self().pos_x[actor];
    const int y = observation.self().pos_y[actor];
    if (x != state.room_target_x || y != state.room_target_y) {
        action.units[actor] = move_toward(
            x, y, state.room_target_x, state.room_target_y);
    } else if (observation.hour == 23) {
        action.units[actor] = {kag::OP_DROP, kag::N_ITEMS, 1};
        int needed = std::max(0, total - 100);
        for (const int item : ROOM_PRIORITY) {
            const int available = std::max<int>(
                0, observation.own.inv[actor][item] - existing_sell(action, item));
            const int quantity = std::min(needed, available);
            if (quantity <= 0) continue;
            const int before_orders = action.n_orders;
            const int before_sell = existing_sell(action, item);
            add_or_merge_sell(action, item, quantity);
            if (action.n_orders == before_orders &&
                existing_sell(action, item) == before_sell) continue;
            needed -= quantity;
            if (needed <= 0) break;
        }
    }
}

void room_guard(const kag::agent::AgentObservation& observation,
                int step, kag::Action& action) {
    if (step % 24 != 23) return;
    int carried = 0;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key)
            carried += std::max<int>(0, observation.own.inv[unit]
                [observation.own.inv_keys[unit][key]]);
    int produced = 0;
    int consumed = 0;
    for (int unit = 0; unit < action.n_units; ++unit) {
        const kag::UnitAction& order = action.units[unit];
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        const kag::Tile& tile = observation.self().tiles[y][x];
        if (order.op == kag::OP_HARVEST) {
            produced += std::max<int>(0, tile.yield_units);
        } else if (order.op == kag::OP_COLLECT_FERTILIZER &&
                   tile.fertilizer_available) {
            ++produced;
        } else if (order.op == kag::OP_FEED ||
                   order.op == kag::OP_FERTILIZE) {
            ++consumed;
        } else if (order.op == kag::OP_PLACE &&
                   (order.arg == kag::GOOSE || order.arg == kag::COW ||
                    order.arg == kag::SHEEP)) {
            ++consumed;
        }
    }
    std::array<int, kag::N_PRODUCTS> planned{};
    int planned_buys = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
            planned[order.item] += std::max(0, order.n);
        else if (order.op == kag::M_BUY_PRODUCT || order.op == kag::M_BUY_ANIMAL)
            planned_buys += std::max(0, order.n);
    }
    int actual_sells = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        actual_sells += std::min<int>(observation.own.shed[item], planned[item]);
    int needed = std::max(
        0, observation.own.shed_total + carried + produced - consumed +
           planned_buys - actual_sells - 100);
    for (const int item : ROOM_PRIORITY) {
        const int already = planned[item];
        const int available = std::max<int>(
            0, observation.own.shed[item] - already);
        const int quantity = std::min(needed, available);
        if (quantity <= 0) continue;
        const int before_orders = action.n_orders;
        const int before_sell = existing_sell(action, item);
        add_or_merge_sell(action, item, quantity);
        if (action.n_orders == before_orders &&
            existing_sell(action, item) == before_sell) continue;
        planned[item] += quantity;
        needed -= quantity;
        if (needed <= 0) break;
    }
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

}

kag::agent::AgentInfo Policy::info() {
    return {"public-indar-e279-unchanged-exact-port"};
}

void Policy::reset(const kag::agent::AgentInit&) {
    seats_ = {};
    for (SeatState& state : seats_) {
        state.selector_last_step = -1;
        state.selector_expert = -1;
        state.weed_last_step = -1;
        state.r5_last_step = -1;
        state.md_last_step = -1;
        state.room_last_step = -1;
        state.room_day = -1;
        state.room_actor = -1;
        for (WeedTransaction& transaction : state.weeds)
            transaction.start = -1;
    }
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget&, kag::Action& action) {
    SeatState& state = seats_[observation.player & 1u];
    const int expert = select_expert(observation, state);
    const int step = std::clamp(observation.step, 0, 718);
    tape_action(expert, step, observation.self().n_units, action);
    weed_repair(observation, expert, step, state, action);
    room_evac(observation, state, step, action);
    rank_sell_slots(observation, action);
    r5_counter(observation, state, step, action);
    md_counter(observation, state, step, action);
    room_guard(observation, step, action);
    terminal_liquidation(observation, step, action);
    action.finalize();
}

}
