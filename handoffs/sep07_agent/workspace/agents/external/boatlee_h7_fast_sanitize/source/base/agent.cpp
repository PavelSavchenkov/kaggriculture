#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <tuple>

#include "generated_data.hpp"

namespace league::public_unchanged::boatlee {
namespace {

static_assert(generated::OUTER_SHA256 ==
    "c6f96a8521dc9aa369b6f27e5b36b9d481e5c1688f50c8ca215c3bb53f1f9eb8");
static_assert(generated::MOON_SHA256 ==
    "8267ee69166cf57bcc4dc0d32274848de8518451ce17f20537d8828bccce4008");
static_assert(generated::MUTOY_SHA256 ==
    "438f3e60b3d197c27c6a8b0b9a5c1d8c965c27921adb1684762fd5fa2105f46f");
static_assert(generated::MUNIB_SHA256 ==
    "cf44a74e12dbde6d41addfcf96d1498a8b19bf077c7115bf55063fea297855c1");

enum TapeId : int {
    MOON_10C4S = 0,
    MOON_8C6S,
    MOON_6C8S,
    MOON_FIRST_YARN,
    MOON_SECOND_YARN,
    MOON_LEGACY_10C4S,
    MOON_LEGACY_8C6S,
    MOON_LEGACY_6C8S,
    MOON_LEGACY_FIRST_YARN,
    MOON_LEGACY_SECOND_YARN,
    MUTOY,
    MUNIB,
};

enum Route : int { ROUTE_NONE = 0, ROUTE_MUTOY, ROUTE_MUNIB, ROUTE_MOON };

constexpr int WEED_REPLAY_STEPS = 8;
constexpr int PREEMPT_START = 120;
constexpr int PREEMPT_STOP = 680;
constexpr int PREEMPT_MAX_CLONE_DISTANCE = 6;
constexpr int PREEMPT_MIN_FUTURE_QUANTITY = 4;
constexpr int PREEMPT_MAX_BATCH = 12;
constexpr double ADAPT_DECAY = .999;
constexpr double ADAPT_MIN_EVIDENCE = 1.5;
constexpr std::array<int, 4> PREMIUM{
    kag::STRAWBERRY, kag::MELON, kag::MILK, kag::WOOL};
constexpr std::array<int, 4> MUNIB_FRONT_ITEMS{
    kag::WOOL, kag::MILK, kag::MELON, kag::STRAWBERRY};
constexpr std::array<int, 9> LIQUIDATION{
    kag::CARROT, kag::EGG, kag::FERTILIZER, kag::MELON, kag::MILK,
    kag::STRAWBERRY, kag::TOMATO, kag::WHEAT, kag::WOOL};
constexpr std::array<int, 9> ROOM_PRIORITY{
    kag::WOOL, kag::MILK, kag::EGG, kag::MELON, kag::STRAWBERRY,
    kag::TOMATO, kag::CARROT, kag::FERTILIZER, kag::WHEAT};
constexpr std::array<const char*, kag::N_PRODUCTS> ITEM_NAMES{
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON",
    "EGG", "MILK", "WOOL", "FERTILIZER"};

int premium_index(int item) {
    for (int index = 0; index < static_cast<int>(PREMIUM.size()); ++index)
        if (PREMIUM[index] == item) return index;
    return -1;
}

const generated::TapeStep& tape_step(int tape, int step) {
    const generated::TapeInfo& info = generated::TAPES.at(tape);
    step = std::clamp(step, 0, static_cast<int>(info.n_steps) - 1);
    return generated::STEPS[info.step_offset + step];
}

kag::UnitAction tape_unit(int tape, int step, int unit) {
    const generated::TapeStep& entry = tape_step(tape, step);
    if (unit < 0 || unit >= entry.n_units) return {};
    return generated::UNITS[entry.unit_offset + unit];
}

void tape_action(int tape, int step, int wanted_units, kag::Action& action) {
    action.clear();
    action.n_units = wanted_units;
    std::fill_n(action.units, wanted_units, kag::UnitAction{});
    const generated::TapeStep& entry = tape_step(tape, step);
    const int copied_units = std::min<int>(wanted_units, entry.n_units);
    std::copy_n(generated::UNITS.begin() + entry.unit_offset,
                copied_units, action.units);
    action.n_orders = entry.n_orders;
    if (action.n_orders > 10) std::abort();
    std::copy_n(generated::ORDERS.begin() + entry.order_offset,
                action.n_orders, action.orders);
}

const generated::MarketStep* market_step(int tape, int step) {
    const generated::MarketInfo& info = generated::MARKET_TAPES.at(tape);
    if (step < 0 || step >= info.n_steps) return nullptr;
    return &generated::MARKET_STEPS[info.step_offset + step];
}

bool actions_equal(const kag::Action& left, const kag::Action& right) {
    if (left.n_units != right.n_units || left.n_orders != right.n_orders)
        return false;
    for (int index = 0; index < left.n_units; ++index) {
        const kag::UnitAction& a = left.units[index];
        const kag::UnitAction& b = right.units[index];
        if (a.op != b.op || a.arg != b.arg || a.n != b.n) return false;
    }
    for (int index = 0; index < left.n_orders; ++index) {
        const kag::Order& a = left.orders[index];
        const kag::Order& b = right.orders[index];
        if (a.op != b.op || a.item != b.item || a.n != b.n) return false;
    }
    return true;
}

int count_animal(const kag::agent::PublicFarm& farm, int animal) {
    int result = 0;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            result += tile.has_animal && tile.what == animal;
        }
    return result;
}

bool has_goose_or_coop(const kag::agent::PublicFarm& farm) {
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            if (tile.kind == kag::T_COOP ||
                (tile.has_animal && tile.what == kag::GOOSE)) return true;
        }
    return false;
}

std::array<int, 11> public_signature(const kag::agent::PublicFarm& farm) {
    std::array<int, 11> result{};
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x) {
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

bool is_shop(const kag::agent::AgentObservation& observation,
             int index, int shop) {
    return index < observation.n_shops && observation.shops[index] == shop;
}

int moon_route_tape(const kag::agent::AgentObservation& observation,
                    MoonSeatState& state) {
    int current = MOON_8C6S;
    if (is_shop(observation, 0, kag::SHOP_YARN_STORE)) {
        current = MOON_FIRST_YARN;
    } else if (is_shop(observation, 1, kag::SHOP_YARN_STORE)) {
        current = MOON_SECOND_YARN;
    } else if (is_shop(observation, 2, kag::SHOP_YARN_STORE)) {
        current = MOON_6C8S;
    } else {
        for (int index = 0; index < std::min(3, observation.n_shops); ++index)
            if (observation.shops[index] == kag::SHOP_PIZZA_SHOP ||
                observation.shops[index] == kag::SHOP_ICE_CREAM_SHOP ||
                observation.shops[index] == kag::SHOP_SMOOTHIE_SHOP) {
                current = MOON_10C4S;
                break;
            }
    }
    if (observation.step == 0) state.layout = -1;
    if (state.layout < 0 && observation.step >= 24 && observation.step < 72) {
        const auto& opponent = observation.opponent();
        const bool legacy =
            public_signature(opponent)[kag::WHEAT] == 5 &&
            public_signature(opponent)[kag::MELON] == 5 &&
            count_animal(opponent, kag::COW) == 1 &&
            count_animal(opponent, kag::SHEEP) == 4 &&
            public_signature(opponent)[8] == 0 && opponent.money <= 12;
        state.layout = legacy ? 1 : 0;
    }
    if (state.layout != 1) return current;
    if (current == MOON_10C4S) return MOON_LEGACY_10C4S;
    if (current == MOON_8C6S) return MOON_LEGACY_8C6S;
    if (current == MOON_6C8S) return MOON_LEGACY_6C8S;
    if (current == MOON_FIRST_YARN) return MOON_LEGACY_FIRST_YARN;
    return MOON_LEGACY_SECOND_YARN;
}

void reset_weeds(int& last_step,
                 std::array<WeedTransaction, kag::MAX_UNITS>& weeds) {
    last_step = -1;
    weeds.fill({});
    for (WeedTransaction& transaction : weeds) transaction.start = -1;
}

void weed_repair(const kag::agent::AgentObservation& observation,
                 int tape, int step, int& last_step,
                 std::array<WeedTransaction, kag::MAX_UNITS>& weeds,
                 kag::Action& action) {
    if (step == 0 || step < last_step) reset_weeds(last_step, weeds);
    last_step = step;
    for (int unit = action.n_units; unit < kag::MAX_UNITS; ++unit)
        weeds[unit] = {};
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = weeds[unit];
        if (!transaction.active) continue;
        const int age = step - transaction.start;
        if (age == 1) {
            action.units[unit] = transaction.intended;
        } else if (age >= 2 && age <= 1 + WEED_REPLAY_STEPS) {
            action.units[unit] = tape_unit(tape, step - 1, unit);
        } else {
            transaction = {};
        }
    }
    const auto& farm = observation.self();
    for (int unit = 0; unit < action.n_units; ++unit) {
        WeedTransaction& transaction = weeds[unit];
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

std::array<int, kag::N_ITEMS> projected_shed(
    const kag::agent::AgentObservation& observation, const kag::Action& action) {
    std::array<int, kag::N_ITEMS> result{};
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        result[item] = std::max<int>(0, observation.own.shed[item]);
        total += result[item];
    }
    const auto& farm = observation.self();
    for (int unit = 0; unit < action.n_units; ++unit) {
        const int x = farm.pos_x[unit];
        const int y = farm.pos_y[unit];
        if (!((x == 4 || x == 5) && (y == 4 || y == 5))) continue;
        const kag::UnitAction& unit_action = action.units[unit];
        if (unit_action.op == kag::OP_DROP) {
            for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key) {
                const int item = observation.own.inv_keys[unit][key];
                const int quantity = std::min<int>(
                    std::max(0, 100 - total),
                    std::max<int>(0, observation.own.inv[unit][item]));
                result[item] += quantity;
                total += quantity;
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
            const int quantity = std::min({
                std::max(0, unit_action.n),
                std::max<int>(0, observation.own.inv[unit][item]),
                std::max(0, 100 - total)});
            result[item] += quantity;
            total += quantity;
        }
    }
    return result;
}

int town_demand_now(const kag::agent::AgentObservation& observation,
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

int tape_planned_sell(int tape, int step, int item) {
    const generated::TapeInfo& info = generated::TAPES.at(tape);
    if (step < 0 || step >= info.n_steps) return 0;
    const generated::TapeStep& entry =
        generated::STEPS[info.step_offset + step];
    int result = 0;
    for (int index = 0; index < entry.n_orders; ++index) {
        const kag::Order& order = generated::ORDERS[entry.order_offset + index];
        if (order.op == kag::M_SELL && order.item == item)
            result += std::max(0, order.n);
    }
    return result;
}

void tape_policy(const kag::agent::AgentObservation& observation, int tape,
                 bool front_run, TapeSeatState& state, kag::Action& action) {
    const int step = std::clamp(observation.step, 0,
        static_cast<int>(generated::TAPES[tape].n_steps) - 1);
    tape_action(tape, step, observation.self().n_units, action);
    weed_repair(observation, tape, step, state.weed_last_step,
                state.weeds, action);
    if (step == 0 || step < state.front_last_step) {
        state.front_last_step = step;
        state.due_step = -1;
        state.due.fill(0);
    }
    state.front_last_step = step;
    if (state.due_step >= 0 && state.due_step < step) {
        state.due_step = -1;
        state.due.fill(0);
    }
    if (state.due_step == step) {
        int kept = 0;
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order order = action.orders[index];
            if (order.op == kag::M_SELL && state.due[order.item] > 0) {
                const int reduction = std::min(
                    std::max(0, order.n), state.due[order.item]);
                order.n -= reduction;
                state.due[order.item] -= reduction;
                if (order.n <= 0) continue;
            }
            action.orders[kept++] = order;
        }
        action.n_orders = kept;
        state.due_step = -1;
        state.due.fill(0);
    }
    if (!front_run) return;
    std::array<int, kag::N_PRODUCTS> moved{};
    for (const int item : MUNIB_FRONT_ITEMS) {
        const int target = tape_planned_sell(tape, step + 1, item);
        if (target <= 0 || town_demand_now(observation, item, step) > 0)
            continue;
        const int reserve = pickup_reserve(action, item) +
                            existing_sell(action, item);
        const int quantity = std::min(
            target, std::max<int>(0, observation.own.shed[item] - reserve));
        if (quantity <= 0) continue;
        int existing = -1;
        for (int index = 0; index < action.n_orders; ++index)
            if (action.orders[index].op == kag::M_SELL &&
                action.orders[index].item == item) {
                existing = index;
                break;
            }
        if (existing >= 0) {
            action.orders[existing].n =
                std::max(0, action.orders[existing].n) + quantity;
        } else if (action.n_orders < 10) {
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item), quantity};
        } else {
            continue;
        }
        moved[item] += quantity;
    }
    if (std::any_of(moved.begin(), moved.end(),
                    [](int value) { return value > 0; })) {
        state.due_step = step + 1;
        state.due = moved;
    }
}

void reset_race(RaceState& race) {
    race = {};
    race.last_step = -1;
    race.horizon.fill(1);
    race.policy_horizon = 1;
}

int race_town_drain(int step, const RaceState& race, int item) {
    int result = step % 24 == 0 ? 1 : 0;
    if (step % 4 != 0) return result;
    for (int index = 0; index < race.n_shops; ++index) {
        const int shop = race.shops[index];
        if (kag::SHOP_MASK[shop] & (uint16_t{1} << item))
            result += kag::SHOP_MULT[shop];
    }
    return result;
}

void observe_opponent_market(const kag::agent::AgentObservation& observation,
                             MoonSeatState& state, int tape, int step) {
    RaceState& race = state.race;
    if (step == 0 || step < race.last_step) reset_race(race);
    race.policy_evidence *= ADAPT_DECAY;
    for (int horizon = 1; horizon <= 6; ++horizon)
        race.policy_scores[horizon] *= ADAPT_DECAY;
    for (int product = 0; product < 4; ++product) {
        race.evidence[product] *= ADAPT_DECAY;
        for (int horizon = 1; horizon <= 6; ++horizon)
            race.scores[product][horizon] *= ADAPT_DECAY;
    }
    if (race.has_inventory && race.last_step == step - 1 &&
        clone_distance(observation) <= PREEMPT_MAX_CLONE_DISTANCE) {
        for (int product = 0; product < 4; ++product) {
            const int item = PREMIUM[product];
            if (race.prices[item] <= 1 || observation.market.prices[item] <= 1)
                continue;
            const int delta = observation.market.inventory[item] -
                              race.inventory[item];
            const int supply = delta + race_town_drain(
                race.last_step, race, item) - race.own_sells[item];
            const int extra = supply - tape_planned_sell(
                tape, race.last_step, item);
            if (extra < PREEMPT_MIN_FUTURE_QUANTITY) continue;
            race.evidence[product] += 1;
            race.policy_evidence += 1;
            for (int horizon = 1; horizon <= 6; ++horizon) {
                const int expected = tape_planned_sell(
                    tape, race.last_step + horizon, item);
                if (expected > 0) {
                    const double similarity = static_cast<double>(
                        std::min(extra, expected)) / std::max(extra, expected);
                    race.scores[product][horizon] += 1 + similarity;
                    race.policy_scores[horizon] += 1 + similarity;
                } else {
                    race.scores[product][horizon] -= .15;
                    race.policy_scores[horizon] -= .15;
                }
            }
            if (race.evidence[product] >= ADAPT_MIN_EVIDENCE) {
                int best = 1;
                for (int horizon = 1; horizon <= 6; ++horizon) {
                    if (race.scores[product][horizon] >
                        race.scores[product][best]) best = horizon;
                }
                int runner = best == 1 ? 2 : 1;
                for (int horizon = 1; horizon <= 6; ++horizon)
                    if (horizon != best && race.scores[product][horizon] >
                        race.scores[product][runner]) runner = horizon;
                if (race.scores[product][best] >=
                    race.scores[product][runner] + .25)
                    race.horizon[product] = std::min(6, best + 1);
            }
        }
    }
    if (race.policy_evidence >= ADAPT_MIN_EVIDENCE) {
        int best = 1;
        for (int horizon = 1; horizon <= 6; ++horizon)
            if (race.policy_scores[horizon] > race.policy_scores[best])
                best = horizon;
        int runner = best == 1 ? 2 : 1;
        for (int horizon = 1; horizon <= 6; ++horizon)
            if (horizon != best && race.policy_scores[horizon] >
                race.policy_scores[runner]) runner = horizon;
        if (race.policy_scores[best] >= race.policy_scores[runner] + .25) {
            race.policy_horizon = std::min(6, best + 1);
            for (int product = 0; product < 4; ++product)
                if (race.horizon[product] == 1)
                    race.horizon[product] = race.policy_horizon;
        }
    }
    race.last_step = step;
    race.has_inventory = true;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        race.inventory[item] = observation.market.inventory[item];
        race.prices[item] = observation.market.prices[item];
    }
    race.n_shops = observation.n_shops;
    std::copy_n(observation.shops, race.n_shops, race.shops.begin());
}

void reset_shift(MoonSeatState& state, int step) {
    state.shift_last_step = step;
    for (auto& row : state.debts) row.fill(0);
}

void repay_shift(MoonSeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.shift_last_step) reset_shift(state, step);
    state.shift_last_step = step;
    if (step < 0 || step >= static_cast<int>(state.debts.size())) return;
    std::array<int, 4> due = state.debts[step];
    state.debts[step].fill(0);
    if (std::all_of(due.begin(), due.end(),
                    [](int value) { return value == 0; })) return;
    int kept = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        kag::Order order = action.orders[index];
        const int product = premium_index(order.item);
        if (order.op == kag::M_SELL && product >= 0 && due[product] > 0) {
            const int reduction = std::min(std::max(0, order.n), due[product]);
            order.n -= reduction;
            due[product] -= reduction;
            if (order.n <= 0) continue;
        }
        action.orders[kept++] = order;
    }
    action.n_orders = kept;
}

double demand_per_day(const kag::agent::AgentObservation& observation,
                      int item) {
    double demand = 0;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (kag::SHOP_MASK[shop] & (uint16_t{1} << item))
            demand += 6.0 * kag::SHOP_MULT[shop];
    }
    if (item != kag::FERTILIZER) demand += 1;
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
    const double later = kag::market_price(item, inventory + quantity);
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
    std::array<Row, 10> rows;
    int n_rows = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item < kag::N_PRODUCTS)
            rows[n_rows++] = {
                order_score(observation, action.orders[index]),
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

struct PreemptChoice {
    double value;
    int item;
    int target;
    int horizon;
};

bool choice_less(const PreemptChoice& left, const PreemptChoice& right) {
    if (left.value != right.value) return left.value < right.value;
    const int names = std::string_view(ITEM_NAMES[left.item]).compare(
        ITEM_NAMES[right.item]);
    if (names != 0) return names < 0;
    if (left.target != right.target) return left.target < right.target;
    return left.horizon < right.horizon;
}

void preempt_shift(const kag::agent::AgentObservation& observation,
                   MoonSeatState& state, int tape, int step,
                   kag::Action& action) {
    const int distance = clone_distance(observation);
    if (step < PREEMPT_START || step >= PREEMPT_STOP ||
        distance > PREEMPT_MAX_CLONE_DISTANCE ||
        action.n_orders >= 10) return;
    std::array<int, kag::N_ITEMS> remaining = projected_shed(observation, action);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_ITEMS)
            remaining[order.item] = std::max(
                0, remaining[order.item] - std::max(0, order.n));
    }
    std::array<PreemptChoice, 4> choices;
    int n_choices = 0;
    for (int product = 0; product < 4; ++product) {
        const int item = PREMIUM[product];
        int preferred = state.race.horizon[product];
        if (step >= PREEMPT_START && distance <= 2)
            preferred = std::max(3, preferred);
        for (int horizon = preferred; horizon >= 1; --horizon) {
            const int future = tape_planned_sell(tape, step + horizon, item);
            if (future < PREEMPT_MIN_FUTURE_QUANTITY) continue;
            const int target = std::min({
                std::max(0, remaining[item]), future, PREEMPT_MAX_BATCH,
                std::max(1, static_cast<int>(std::nearbyint(future * 1.0)))});
            if (target > 0)
                choices[n_choices++] = {
                    static_cast<double>(observation.market.prices[item]) * target,
                    item, target, horizon};
            break;
        }
    }
    int best = -1;
    for (int index = 0; index < n_choices; ++index)
        if (choices[index].horizon > 1 &&
            (best < 0 || choice_less(choices[best], choices[index])))
            best = index;
    const auto emit = [&](const PreemptChoice& choice) {
        if (action.n_orders >= 10) return;
        action.orders[action.n_orders++] = {
            kag::M_SELL, static_cast<uint8_t>(choice.item), choice.target};
        remaining[choice.item] = std::max(
            0, remaining[choice.item] - choice.target);
        const int due_step = step + choice.horizon;
        if (due_step < static_cast<int>(state.debts.size()))
            state.debts[due_step][premium_index(choice.item)] += choice.target;
    };
    if (best >= 0) {
        emit(choices[best]);
    } else {
        for (int index = 0; index < n_choices && action.n_orders < 10; ++index)
            emit(choices[index]);
    }
}

int auxiliary_planned_sell(int market_tape, int step, int item) {
    const generated::MarketStep* entry = market_step(market_tape, step);
    if (entry == nullptr) return 0;
    int result = 0;
    for (int index = 0; index < entry->n_orders; ++index) {
        const kag::Order& order =
            generated::MARKET_ORDERS[entry->order_offset + index];
        if (order.op == kag::M_SELL && order.item == item)
            result += std::max(0, order.n);
    }
    return result;
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
                MoonSeatState& state, int step, kag::Action& action) {
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
    if (!state.r5_target || market_step(0, step + 3) == nullptr) return;
    for (const int item : std::array<int, 4>{
             kag::MELON, kag::MILK, kag::STRAWBERRY, kag::WOOL}) {
        const int planned = auxiliary_planned_sell(0, step + 3, item);
        if (planned <= 0 || town_demand_now(observation, item, step) > 0 ||
            town_demand_now(observation, item, step + 1) > 0) continue;
        const int available = std::max<int>(
            0, observation.own.shed[item] - existing_sell(action, item) -
               pickup_reserve(action, item));
        const int quantity = std::min(
            available, std::max(1, static_cast<int>(std::nearbyint(planned * .5))));
        if (quantity > 0) add_or_merge_sell(action, item, quantity);
    }
    if (action.n_orders > 10) action.n_orders = 10;
}

void md_counter(const kag::agent::AgentObservation& observation,
                MoonSeatState& state, int step, kag::Action& action) {
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
    if (!state.md_target || market_step(1, step + 1) == nullptr) return;
    for (const int item : std::array<int, 4>{
             kag::MELON, kag::MILK, kag::STRAWBERRY, kag::WOOL}) {
        const int target = auxiliary_planned_sell(1, step + 1, item);
        if (target <= 0) continue;
        const int available = std::max<int>(
            0, observation.own.shed[item] - existing_sell(action, item) -
               pickup_reserve(action, item));
        const int quantity = std::min(
            available, std::max(1, static_cast<int>(std::nearbyint(target * 2.0))));
        if (quantity > 0) add_or_merge_sell(action, item, quantity);
    }
    if (action.n_orders > 10) action.n_orders = 10;
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

void room_evac(const kag::agent::AgentObservation& observation,
               MoonSeatState& state, int step, kag::Action& action) {
    if (step < 648) return;
    if (step == 0 || step < state.room_last_step ||
        observation.day != state.room_day) {
        state.room_last_step = step;
        state.room_day = observation.day;
        state.room_actor = -1;
    }
    state.room_last_step = step;
    if (observation.hour < 21) return;
    int total = observation.own.shed_total;
    for (int unit = 0; unit < observation.self().n_units; ++unit)
        for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key)
            total += std::max<int>(0, observation.own.inv[unit]
                [observation.own.inv_keys[unit][key]]);
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
            const int before = action.n_orders;
            add_or_merge_sell(action, item, quantity);
            if (action.n_orders == before && existing_sell(action, item) < quantity)
                continue;
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
    std::array<int, kag::N_PRODUCTS> planned_sells{};
    int planned_buys = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
            planned_sells[order.item] += std::max(0, order.n);
        else if (order.op == kag::M_BUY_PRODUCT || order.op == kag::M_BUY_ANIMAL)
            planned_buys += std::max(0, order.n);
    }
    int actual_sells = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        actual_sells += std::min<int>(
            observation.own.shed[item], planned_sells[item]);
    int needed = std::max(
        0, observation.own.shed_total + carried + produced - consumed +
           planned_buys - actual_sells - 100);
    for (const int item : ROOM_PRIORITY) {
        const int already = planned_sells[item];
        const int available = std::max<int>(
            0, observation.own.shed[item] - already);
        const int quantity = std::min(needed, available);
        if (quantity <= 0) continue;
        const int before = action.n_orders;
        add_or_merge_sell(action, item, quantity);
        if (action.n_orders == before && existing_sell(action, item) == already)
            continue;
        planned_sells[item] += quantity;
        needed -= quantity;
        if (needed <= 0) break;
    }
}

void tomato_patch(const kag::agent::AgentObservation& observation,
                  MoonSeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.tomato_last_step) {
        state.tomato_last_step = step;
        state.tomato_active = false;
        state.tomato_scheduled_plants = 0;
        state.tomato_seed_debt = 0;
    }
    state.tomato_last_step = step;
    if (step == 216) {
        int matches = 0;
        for (int index = 0; index < std::min(3, observation.n_shops); ++index)
            matches += observation.shops[index] == kag::SHOP_FARMERS_MARKET ||
                       observation.shops[index] == kag::SHOP_PIZZA_SHOP;
        state.tomato_active = matches >= 2;
    }
    if (!state.tomato_active) return;
    if (step == 264) {
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order& order = action.orders[index];
            if (order.op == kag::M_BUY_SEED && order.item == kag::STRAWBERRY &&
                order.n >= 3) {
                order.n -= 3;
                state.tomato_seed_debt = 3;
                break;
            }
        }
    }
    if (step == 265 && state.tomato_seed_debt > 0 && action.n_orders < 10) {
        action.orders[action.n_orders++] = {
            kag::M_BUY_SEED, kag::TOMATO, state.tomato_seed_debt};
        state.tomato_seed_debt = 0;
    }
    if (step >= 271 && step <= 286) {
        int remaining = std::max(0, 3 - state.tomato_scheduled_plants);
        int changed = 0;
        for (int unit = 0; unit < action.n_units && changed < remaining; ++unit)
            if (action.units[unit].op == kag::OP_PLANT &&
                action.units[unit].arg == kag::STRAWBERRY) {
                action.units[unit].arg = kag::TOMATO;
                ++changed;
            }
        state.tomato_scheduled_plants += changed;
    }
    for (int unit = 0; unit < action.n_units; ++unit)
        if (action.units[unit].op == kag::OP_PLACE &&
            action.units[unit].arg == kag::STRAWBERRY &&
            observation.own.inv[unit][kag::TOMATO] > 0)
            action.units[unit].arg = kag::TOMATO;
    int kept = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order order = action.orders[index];
        if (order.op == kag::M_SELL && order.item == kag::TOMATO) continue;
        action.orders[kept++] = order;
    }
    action.n_orders = kept;
}

void egg_patch(const kag::agent::AgentObservation& observation,
               MoonSeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.egg_last_step) {
        state.egg_last_step = step;
        state.egg_active = false;
    }
    state.egg_last_step = step;
    if (step == 264) {
        int egg_shops = 0;
        bool yarn = false;
        for (int index = 0; index < std::min(3, observation.n_shops); ++index) {
            egg_shops += observation.shops[index] == kag::SHOP_BAKERY ||
                         observation.shops[index] == kag::SHOP_BRUNCH_SPOT;
            yarn |= observation.shops[index] == kag::SHOP_YARN_STORE;
        }
        const bool bbb = observation.n_shops >= 3 &&
            is_shop(observation, 0, kag::SHOP_BAKERY) &&
            is_shop(observation, 1, kag::SHOP_BAKERY) &&
            is_shop(observation, 2, kag::SHOP_BAKERY);
        const bool ibb = observation.n_shops >= 3 &&
            is_shop(observation, 0, kag::SHOP_ICE_CREAM_SHOP) &&
            is_shop(observation, 1, kag::SHOP_BAKERY) &&
            is_shop(observation, 2, kag::SHOP_BAKERY);
        const bool bbf = observation.n_shops >= 3 &&
            is_shop(observation, 0, kag::SHOP_BRUNCH_SPOT) &&
            is_shop(observation, 1, kag::SHOP_BRUNCH_SPOT) &&
            is_shop(observation, 2, kag::SHOP_FARMERS_MARKET) &&
            clone_distance(observation) == 0;
        state.egg_active = egg_shops >= 2 && !yarn && !bbb && !ibb && !bbf &&
                           !has_goose_or_coop(observation.opponent());
    }
    if (!state.egg_active || step < 264 || step > 275) return;
    for (int unit = 0; unit < action.n_units; ++unit) {
        kag::UnitAction& order = action.units[unit];
        if ((step == 266 || step == 269) &&
            order.op == kag::OP_BUILD_PASTURE) {
            order.op = kag::OP_BUILD_COOP;
        } else if (step >= 267 && order.op >= kag::OP_PICKUP &&
                   (order.op == kag::OP_PICKUP || order.op == kag::OP_PLACE) &&
                   (order.arg == kag::COW || order.arg == kag::SHEEP)) {
            order.arg = kag::GOOSE;
        }
    }
    if (step == 264)
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order& order = action.orders[index];
            if (order.op == kag::M_BUY_ANIMAL &&
                (order.item == kag::COW || order.item == kag::SHEEP))
                order.item = kag::GOOSE;
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

kag::UnitAction terminal_move(const kag::agent::PublicFarm& farm,
                              int x, int y, int tx, int ty) {
    struct Candidate { uint8_t op; int x; int y; };
    std::array<Candidate, 4> choices{};
    int size = 0;
    if (tx < x) choices[size++] = {kag::OP_WEST, x - 1, y};
    if (tx > x) choices[size++] = {kag::OP_EAST, x + 1, y};
    if (ty < y) choices[size++] = {kag::OP_NORTH, x, y - 1};
    if (ty > y) choices[size++] = {kag::OP_SOUTH, x, y + 1};
    for (int index = 0; index < size; ++index) {
        const Candidate& choice = choices[index];
        if (choice.x >= 0 && choice.x < kag::BOARD && choice.y >= 0 &&
            choice.y < kag::BOARD &&
            farm.tiles[choice.y][choice.x].kind != kag::T_LOCKED)
            return {choice.op, kag::N_ITEMS, 1};
    }
    return {};
}

void terminal_action(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    action.clear();
    action.n_units = observation.self().n_units;
    std::array<std::array<bool, kag::BOARD>, kag::BOARD> available{};
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            available[y][x] = observation.self().tiles[y][x].yield_units > 0;
    std::array<int, kag::N_PRODUCTS> pending{};
    for (int unit = 0; unit < action.n_units; ++unit) {
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        int load = 0;
        for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key)
            load += std::max<int>(0, observation.own.inv[unit]
                [observation.own.inv_keys[unit][key]]);
        const kag::Tile& tile = observation.self().tiles[y][x];
        if (load > 0 && (x == 4 || x == 5) && (y == 4 || y == 5)) {
            action.units[unit] = {kag::OP_DROP, kag::N_ITEMS, 1};
            for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key) {
                const int item = observation.own.inv_keys[unit][key];
                if (item < kag::N_PRODUCTS)
                    pending[item] += std::max<int>(
                        0, observation.own.inv[unit][item]);
            }
        } else if (tile.yield_units > 0) {
            action.units[unit] = {kag::OP_HARVEST, kag::N_ITEMS, 1};
            available[y][x] = false;
        } else if (load > 0) {
            const auto target = nearest_shed(x, y);
            action.units[unit] = terminal_move(
                observation.self(), x, y, target[0], target[1]);
        } else {
            bool found = false;
            int target_x = 0;
            int target_y = 0;
            int best_distance = 0;
            for (int ty = 0; ty < kag::BOARD; ++ty)
                for (int tx = 0; tx < kag::BOARD; ++tx) {
                    if (!available[ty][tx]) continue;
                    const int distance = std::abs(x - tx) + std::abs(y - ty);
                    if (!found || std::make_tuple(distance, ty, tx) <
                                  std::make_tuple(best_distance, target_y, target_x)) {
                        found = true;
                        best_distance = distance;
                        target_x = tx;
                        target_y = ty;
                    }
                }
            if (found) {
                available[target_y][target_x] = false;
                action.units[unit] = terminal_move(
                    observation.self(), x, y, target_x, target_y);
            } else if (tile.fertilizer_available) {
                action.units[unit] = {
                    kag::OP_COLLECT_FERTILIZER, kag::N_ITEMS, 1};
            } else {
                action.units[unit] = {};
            }
        }
    }
    struct Sale { long long value; int item; int quantity; };
    std::array<Sale, kag::N_PRODUCTS> sales;
    int n_sales = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const int quantity = std::max<int>(
            0, observation.own.shed[item] + pending[item]);
        if (quantity > 0)
            sales[n_sales++] = {
                static_cast<long long>(quantity) *
                    std::max(1, observation.market.prices[item]),
                item, quantity};
    }
    std::sort(sales.begin(), sales.begin() + n_sales,
        [](const Sale& left, const Sale& right) {
            if (left.value != right.value) return left.value > right.value;
            const int names = std::string_view(ITEM_NAMES[left.item]).compare(
                ITEM_NAMES[right.item]);
            if (names != 0) return names > 0;
            return left.quantity > right.quantity;
        });
    for (int index = 0; index < n_sales; ++index) {
        const Sale& sale = sales[index];
        if (action.n_orders >= 10) break;
        action.orders[action.n_orders++] = {
            kag::M_SELL, static_cast<uint8_t>(sale.item), sale.quantity};
    }
}

void record_own_sells(const kag::agent::AgentObservation& observation,
                      MoonSeatState& state, const kag::Action& action) {
    std::array<int, kag::N_ITEMS> remaining = projected_shed(observation, action);
    state.race.own_sells.fill(0);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op != kag::M_SELL || premium_index(order.item) < 0) continue;
        const int quantity = std::min(
            std::max(0, order.n), std::max(0, remaining[order.item]));
        if (quantity > 0) {
            state.race.own_sells[order.item] += quantity;
            remaining[order.item] -= quantity;
        }
    }
}

void moon_policy(const kag::agent::AgentObservation& observation,
                 MoonSeatState& state, kag::Action& action) {
    const int tape = moon_route_tape(observation, state);
    const int step = std::clamp(observation.step, 0,
        static_cast<int>(generated::TAPES[tape].n_steps) - 1);
    observe_opponent_market(observation, state, tape, step);
    if (step >= 708) {
        terminal_action(observation, action);
        return;
    }
    tape_action(tape, step, observation.self().n_units, action);
    weed_repair(observation, tape, step, state.weed_last_step,
                state.weeds, action);
    room_evac(observation, state, step, action);
    repay_shift(state, step, action);
    rank_sell_slots(observation, action);
    preempt_shift(observation, state, tape, step, action);
    r5_counter(observation, state, step, action);
    md_counter(observation, state, step, action);
    tomato_patch(observation, state, step, action);
    egg_patch(observation, state, step, action);
    room_guard(observation, step, action);
    terminal_liquidation(observation, step, action);
    record_own_sells(observation, state, action);
}

void sell_totals(const kag::Action& action,
                 std::array<int, kag::N_PRODUCTS>& totals) {
    totals.fill(0);
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
            totals[order.item] += std::max(0, order.n);
    }
}

void apply_market_delta(kag::Action& action, const kag::Action& base,
                        const kag::Action& shifted) {
    std::array<int, kag::N_PRODUCTS> base_totals{};
    std::array<int, kag::N_PRODUCTS> shifted_totals{};
    sell_totals(base, base_totals);
    sell_totals(shifted, shifted_totals);
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const int delta = shifted_totals[item] - base_totals[item];
        if (delta > 0) {
            int existing = -1;
            for (int index = 0; index < action.n_orders; ++index)
                if (action.orders[index].op == kag::M_SELL &&
                    action.orders[index].item == item) {
                    existing = index;
                    break;
                }
            if (existing >= 0)
                action.orders[existing].n =
                    std::max(0, action.orders[existing].n) + delta;
            else if (action.n_orders < 10)
                action.orders[action.n_orders++] = {
                    kag::M_SELL, static_cast<uint8_t>(item), delta};
        } else if (delta < 0) {
            int remaining = -delta;
            int kept = 0;
            for (int index = 0; index < action.n_orders; ++index) {
                kag::Order order = action.orders[index];
                if (remaining > 0 && order.op == kag::M_SELL &&
                    order.item == item) {
                    const int reduction = std::min(
                        std::max(0, order.n), remaining);
                    order.n -= reduction;
                    remaining -= reduction;
                    if (order.n <= 0) continue;
                }
                action.orders[kept++] = order;
            }
            action.n_orders = kept;
        }
    }
}

}

kag::agent::AgentInfo Policy::info() {
    return {"public-boatlee-v21-r1-unchanged-exact-port"};
}

void Policy::reset(const kag::agent::AgentInit&) {
    seats_ = {};
    for (SeatState& seat : seats_) {
        seat.route = ROUTE_NONE;
        seat.moon.layout = -1;
        reset_weeds(seat.moon.weed_last_step, seat.moon.weeds);
        reset_shift(seat.moon, -1);
        reset_race(seat.moon.race);
        for (TapeSeatState* tape : {
                 &seat.mutoy, &seat.munib_base, &seat.munib_front}) {
            reset_weeds(tape->weed_last_step, tape->weeds);
            tape->front_last_step = -1;
            tape->due_step = -1;
            tape->due.fill(0);
        }
    }
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget&, kag::Action& action) {
    if (observation.player > 1 || observation.self().n_units > kag::MAX_UNITS)
        std::abort();
    SeatState& state = seats_[observation.player];
    const int step = observation.step;
    const double opponent_money = observation.opponent().money;
    const int opponent_hires = observation.opponent().hires_today;
    if (step == 0) {
        state.route = ROUTE_NONE;
        state.market_overlay = false;
        state.has_previous_opponent_money = true;
        state.previous_opponent_money = opponent_money;
        kag::Action moon{};
        kag::Action ignored{};
        moon_policy(observation, state.moon, moon);
        tape_policy(observation, MUTOY, false, state.mutoy, ignored);
        tape_policy(observation, MUNIB, false, state.munib_base, ignored);
        tape_policy(observation, MUNIB, true, state.munib_front, ignored);
        action = moon;
        action.finalize();
        return;
    }
    if (step == 1 && state.route == ROUTE_NONE && opponent_hires >= 4 &&
        opponent_money <= 20) state.route = ROUTE_MUTOY;
    if (state.route == ROUTE_MUTOY) {
        state.previous_opponent_money = opponent_money;
        state.has_previous_opponent_money = true;
        tape_policy(observation, MUTOY, false, state.mutoy, action);
        action.finalize();
        return;
    }
    kag::Action moon{};
    kag::Action munib_base{};
    kag::Action munib{};
    moon_policy(observation, state.moon, moon);
    tape_policy(observation, MUNIB, false, state.munib_base, munib_base);
    tape_policy(observation, MUNIB, true, state.munib_front, munib);
    const double spend = state.has_previous_opponent_money
        ? state.previous_opponent_money - opponent_money : 0;
    if (step == 217 && state.has_previous_opponent_money && spend >= 100)
        state.market_overlay = true;
    state.previous_opponent_money = opponent_money;
    state.has_previous_opponent_money = true;
    if (state.route == ROUTE_NONE && !actions_equal(moon, munib)) {
        const bool bakery = observation.n_shops >= 3 &&
            is_shop(observation, 0, kag::SHOP_BAKERY) &&
            is_shop(observation, 1, kag::SHOP_BAKERY) &&
            is_shop(observation, 2, kag::SHOP_BAKERY);
        const bool extra = observation.n_shops >= 3 &&
            is_shop(observation, 0, kag::SHOP_PET_CAFE) &&
            is_shop(observation, 1, kag::SHOP_ICE_CREAM_SHOP) &&
            is_shop(observation, 2, kag::SHOP_ICE_CREAM_SHOP);
        state.route = step < 200 || bakery || extra ? ROUTE_MUNIB : ROUTE_MOON;
    }
    if (state.route == ROUTE_MUNIB) {
        action = munib;
    } else {
        action = moon;
        if (state.market_overlay) apply_market_delta(action, munib_base, munib);
    }
    action.finalize();
}

}
