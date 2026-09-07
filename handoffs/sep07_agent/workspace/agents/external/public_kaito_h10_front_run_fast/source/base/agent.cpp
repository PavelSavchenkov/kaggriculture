#include "agent.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <limits>

#include "generated_data.hpp"

namespace league::public_unchanged::kaito {
namespace {

static_assert(generated::SOURCE_SHA256 ==
    "d9dc24ce5429ec628ead0621a160bee90725350683d7dfcc4686fcaf511f3aab");

constexpr int MAX_ACTORS = 13;
constexpr int WEED_REPLAY_STEPS = 8;
constexpr double MEMORY_MAX_DISTANCE = 48.0;
constexpr std::array<int, 9> SELLABLE{
    kag::STRAWBERRY, kag::MELON, kag::MILK, kag::WOOL, kag::EGG,
    kag::TOMATO, kag::CARROT, kag::WHEAT, kag::FERTILIZER};
constexpr std::array<double, kag::N_PRODUCTS> GLUT_WEIGHT{
    1.0, 1.0, 1.3, 2.0, 3.6, 1.5, 2.0, 3.2, 1.0};

struct RouteSignature {
    int workers = 0;
    int unlocks = 0;
    std::array<int, 26> positions{};
    std::array<int, 11> counts{};
    std::array<int, 8> yields{};
};

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

void weed_repair(const kag::agent::AgentObservation& observation,
                 SeatState& state, int step, kag::Action& action) {
    if (step == 0 || step < state.last_step) state = {};
    state.last_step = step;
    for (int unit = action.n_units; unit < kag::MAX_UNITS; ++unit)
        state.weeds[unit] = {};
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
        if (!((x == 4 || x == 5) && (y == 4 || y == 5)) ||
            farm.tiles[y][x].kind == kag::T_LOCKED)
            continue;
        const kag::UnitAction& requested = action.units[unit];
        if (requested.op == kag::OP_DROP) {
            for (int key = 0; key < observation.own.inv_nkeys[unit]; ++key) {
                const int item = observation.own.inv_keys[unit][key];
                const int amount = std::min<int>(
                    std::max(0, 100 - total),
                    std::max<int>(0, observation.own.inv[unit][item]));
                projected[item] += amount;
                total += amount;
            }
        } else if (requested.op == kag::OP_PLACE &&
                   requested.arg < kag::N_ITEMS) {
            const int item = requested.arg;
            const kag::Tile& tile = farm.tiles[y][x];
            const bool empty_matching_structure = !tile.has_animal &&
                ((item == kag::GOOSE && tile.kind == kag::T_COOP) ||
                 ((item == kag::COW || item == kag::SHEEP) &&
                  tile.kind == kag::T_PASTURE));
            if (empty_matching_structure) continue;
            const int amount = std::min({
                std::max(0, requested.n),
                std::max<int>(0, observation.own.inv[unit][item]),
                std::max(0, 100 - total)});
            projected[item] += amount;
            total += amount;
        }
    }
    return projected;
}

void safe_market(const kag::agent::AgentObservation& observation,
                 kag::Action& action) {
    std::array<int, kag::N_ITEMS> remaining = projected_shed(observation, action);
    int kept = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        kag::Order order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_ITEMS) {
            order.n = std::min(
                std::max(0, order.n), std::max(0, remaining[order.item]));
            if (order.n <= 0) continue;
            remaining[order.item] = std::max(
                0, remaining[order.item] - order.n);
        }
        if (kept < 10) action.orders[kept++] = order;
    }
    action.n_orders = kept;
}

RouteSignature public_route_signature(const kag::agent::PublicFarm& farm) {
    RouteSignature result;
    result.workers = farm.n_units - 1;
    result.unlocks = (1 << farm.n_quadrants) - 1;
    result.positions.fill(-1);
    const int actors = std::min(farm.n_units, MAX_ACTORS);
    for (int actor = 0; actor < actors; ++actor) {
        result.positions[2 * actor] = farm.pos_x[actor];
        result.positions[2 * actor + 1] = farm.pos_y[actor];
    }
    for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
        const kag::Tile& tile = farm.tiles[y][x];
        if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS) {
            ++result.counts[tile.what];
            result.yields[tile.what] += std::max<int>(0, tile.yield_units);
        }
        if (tile.has_animal) {
            int index = -1;
            if (tile.what == kag::COW) index = 5;
            else if (tile.what == kag::SHEEP) index = 6;
            else if (tile.what == kag::GOOSE) index = 7;
            if (index >= 0) {
                ++result.counts[index];
                result.yields[index] += std::max<int>(0, tile.yield_units);
            }
        }
        if (tile.kind == kag::T_PASTURE) ++result.counts[8];
        if (tile.kind == kag::T_COOP) ++result.counts[9];
        if (tile.kind == kag::T_WEED) ++result.counts[10];
    }
    return result;
}

double signature_distance(const RouteSignature& observed,
                          const generated::Signature& memory) {
    double total = 12.0 * std::abs(observed.workers - memory.workers);
    total += 7.0 * std::popcount(
        static_cast<unsigned>(observed.unlocks ^ memory.unlocks));
    for (int actor = 0; actor < MAX_ACTORS; ++actor) {
        const int offset = 2 * actor;
        if (observed.positions[offset] == -1 &&
            observed.positions[offset + 1] == -1 &&
            memory.positions[offset] == -1 &&
            memory.positions[offset + 1] == -1)
            continue;
        const double weight = actor == 0 ? .8 : .25;
        total += weight * (
            std::abs(observed.positions[offset] - memory.positions[offset]) +
            std::abs(observed.positions[offset + 1] - memory.positions[offset + 1]));
    }
    for (int index = 0; index < 11; ++index)
        total += (index == 10 ? .25 : 3.0) *
            std::abs(observed.counts[index] - memory.counts[index]);
    for (int index = 0; index < 8; ++index)
        total += .15 * std::abs(observed.yields[index] - memory.yields[index]);
    return total;
}

void conditional_reorder(const kag::agent::AgentObservation& observation,
                         int step, kag::Action& action) {
    safe_market(observation, action);
    bool has_sell = false;
    for (int index = 0; index < action.n_orders; ++index)
        has_sell = has_sell || action.orders[index].op == kag::M_SELL;
    if (!has_sell) return;
    const RouteSignature observed = public_route_signature(observation.opponent());
    double best_distance = std::numeric_limits<double>::infinity();
    int best_prototype = -1;
    for (int prototype = 0; prototype < generated::PROTOTYPES; ++prototype) {
        const int offset = prototype * generated::STEPS.size() + step;
        const double distance = signature_distance(
            observed, generated::SIGNATURES[offset]);
        if (distance < best_distance) {
            best_distance = distance;
            best_prototype = prototype;
        }
    }
    if (best_prototype < 0 || best_distance > MEMORY_MAX_DISTANCE) return;
    const uint16_t predicted = generated::SALES_MASKS[
        best_prototype * generated::STEPS.size() + step];
    std::array<kag::Order, 16> reordered{};
    int count = 0;
    std::array<bool, 16> moved{};
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS &&
            (predicted & (uint16_t{1} << order.item))) {
            reordered[count++] = order;
            moved[index] = true;
        }
    }
    for (int index = 0; index < action.n_orders; ++index)
        if (!moved[index]) reordered[count++] = action.orders[index];
    action.n_orders = std::min(count, 10);
    std::copy_n(reordered.begin(), action.n_orders, action.orders);
}

std::array<double, kag::N_PRODUCTS> opponent_exposure(
    const kag::agent::PublicFarm& opponent) {
    std::array<double, kag::N_PRODUCTS> result{};
    for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
        const kag::Tile& tile = opponent.tiles[y][x];
        if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS)
            result[tile.what] += std::max(1.0, static_cast<double>(tile.yield_units));
        if (tile.has_animal) {
            int product = -1;
            if (tile.what == kag::COW) product = kag::MILK;
            else if (tile.what == kag::SHEEP) product = kag::WOOL;
            else if (tile.what == kag::GOOSE) product = kag::EGG;
            if (product >= 0)
                result[product] += 1.0 + std::max(
                    0.0, static_cast<double>(tile.yield_units));
        }
        if (tile.fertilizer_available) result[kag::FERTILIZER] += 1.0;
    }
    return result;
}

void terminal_market(const kag::agent::AgentObservation& observation,
                     kag::Action& action) {
    const auto shed = projected_shed(observation, action);
    const auto exposure = opponent_exposure(observation.opponent());
    struct Row { double score; int index; int item; int quantity; };
    std::array<Row, SELLABLE.size()> rows{};
    int n_rows = 0;
    for (int index = 0; index < static_cast<int>(SELLABLE.size()); ++index) {
        const int item = SELLABLE[index];
        const int quantity = std::max(0, shed[item]);
        if (quantity <= 0) continue;
        const double score = (1.0 + exposure[item]) * GLUT_WEIGHT[item] *
            std::max(1.0, static_cast<double>(observation.market.prices[item])) *
            std::log1p(quantity);
        rows[n_rows++] = {score, index, item, quantity};
    }
    std::sort(rows.begin(), rows.begin() + n_rows,
              [](const Row& left, const Row& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.index < right.index;
    });
    action.n_orders = std::min(10, n_rows);
    for (int index = 0; index < action.n_orders; ++index)
        action.orders[index] = {kag::M_SELL,
            static_cast<uint8_t>(rows[index].item), rows[index].quantity};
}

}

kag::agent::AgentInfo Policy::info() {
    return {"public-kaito-177-180-exact-port"};
}

void Policy::reset(const kag::agent::AgentInit&) {
    seats_ = {};
    for (SeatState& state : seats_) state.last_step = -1;
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget&, kag::Action& action) {
    if (observation.player > 1 || observation.self().n_units > kag::MAX_UNITS)
        std::abort();
    const int step = std::clamp(
        observation.step, 0, static_cast<int>(generated::STEPS.size()) - 1);
    tape_action(step, observation.self().n_units, action);
    weed_repair(observation, seats_[observation.player], step, action);
    safe_market(observation, action);
    conditional_reorder(observation, step, action);
    if (step == 718) terminal_market(observation, action);
    action.finalize();
}

}
