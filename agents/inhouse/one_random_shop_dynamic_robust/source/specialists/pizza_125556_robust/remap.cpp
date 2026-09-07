#include "agents/inhouse/one_random_shop_dynamic_robust/source/specialists/pizza_125556_robust/remap.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <numeric>
#include <vector>

namespace kag::agents::pizza_125556_robust::pizza {
namespace {

enum class ContractOp : uint8_t { Pickup, Build, Place };

struct AnimalUnitContract {
    int16_t step;
    int8_t unit;
    uint8_t group;
    ContractOp op;
    uint8_t n;
};

inline constexpr std::array<int, 15> GROUP_START{
    0, 3, 4, 5, 6, 7, 9, 10, 11, 13, 14, 16, 18, 21, 22};

inline constexpr std::array<AnimalUnitContract, 69> ANIMAL_UNIT_CONTRACTS{{
    {2,1,0,ContractOp::Pickup,2}, {6,1,0,ContractOp::Build,1},
    {7,1,0,ContractOp::Place,1}, {20,1,0,ContractOp::Build,1},
    {21,1,0,ContractOp::Place,1}, {2,3,0,ContractOp::Pickup,1},
    {16,3,0,ContractOp::Build,1}, {17,3,0,ContractOp::Place,1},
    {2,0,1,ContractOp::Pickup,1}, {17,0,1,ContractOp::Build,1},
    {18,0,1,ContractOp::Place,1},
    {26,0,2,ContractOp::Pickup,1}, {27,0,2,ContractOp::Build,1},
    {28,0,2,ContractOp::Place,1},
    {74,0,3,ContractOp::Pickup,1}, {81,0,3,ContractOp::Build,1},
    {82,0,3,ContractOp::Place,1},
    {97,1,4,ContractOp::Pickup,1}, {103,1,4,ContractOp::Build,1},
    {104,1,4,ContractOp::Place,1},
    {122,6,5,ContractOp::Pickup,1}, {132,6,5,ContractOp::Build,1},
    {133,6,5,ContractOp::Place,1}, {123,0,5,ContractOp::Pickup,1},
    {134,0,5,ContractOp::Build,1}, {135,0,5,ContractOp::Place,1},
    {146,0,6,ContractOp::Pickup,1}, {154,0,6,ContractOp::Build,1},
    {155,0,6,ContractOp::Place,1},
    {169,2,7,ContractOp::Pickup,1}, {176,2,7,ContractOp::Build,1},
    {177,2,7,ContractOp::Place,1},
    {194,0,8,ContractOp::Pickup,1}, {211,0,8,ContractOp::Build,1},
    {212,0,8,ContractOp::Place,1}, {194,4,8,ContractOp::Pickup,1},
    {207,4,8,ContractOp::Build,1}, {208,4,8,ContractOp::Place,1},
    {218,2,9,ContractOp::Pickup,1}, {224,2,9,ContractOp::Build,1},
    {225,2,9,ContractOp::Place,1},
    {242,3,10,ContractOp::Pickup,1}, {246,3,10,ContractOp::Build,1},
    {247,3,10,ContractOp::Place,1}, {242,8,10,ContractOp::Pickup,1},
    {253,8,10,ContractOp::Build,1}, {254,8,10,ContractOp::Place,1},
    {266,1,11,ContractOp::Pickup,1}, {280,1,11,ContractOp::Build,1},
    {281,1,11,ContractOp::Place,1}, {267,5,11,ContractOp::Pickup,1},
    {276,5,11,ContractOp::Build,1}, {277,5,11,ContractOp::Place,1},
    {267,1,12,ContractOp::Pickup,2}, {269,1,12,ContractOp::Build,1},
    {270,1,12,ContractOp::Place,1}, {275,1,12,ContractOp::Build,1},
    {276,1,12,ContractOp::Place,1}, {268,5,12,ContractOp::Pickup,1},
    {271,5,12,ContractOp::Build,1}, {272,5,12,ContractOp::Place,1},
    {291,8,13,ContractOp::Pickup,1}, {296,8,13,ContractOp::Build,1},
    {297,8,13,ContractOp::Place,1},
    {411,10,14,ContractOp::Pickup,1}, {412,10,14,ContractOp::Build,1},
    {413,10,14,ContractOp::Place,1},
}};

bool animal_item(int item) {
    return item >= kag::GOOSE && item <= kag::SHEEP;
}

int carried_animal(const kag::agent::AgentObservation& observation, int unit) {
    for (int item = kag::GOOSE; item <= kag::SHEEP; ++item)
        if (observation.own.inv[unit][item] > 0) return item;
    return -1;
}

void append_order(kag::Action& action, uint8_t op, uint8_t item, int n) {
    if (n <= 0 || action.n_orders >= 10) return;
    action.orders[action.n_orders++] = {op, item, n};
}

}  // namespace

bool valid(const Genome& genome) {
    return genome.cows <= SOURCE_ANIMALS &&
        genome.sheep <= SOURCE_ANIMALS - genome.cows &&
        genome.animal_order <= 2 && genome.tomato_mode <= 1 &&
        genome.market_mode <= 2 &&
        genome.sale_period >= 1 && genome.sale_period <= 24 &&
        genome.milk_floor >= 1 && genome.milk_floor <= 1000 &&
        genome.tomato_floor >= 1 && genome.tomato_floor <= 1000 &&
        genome.shed_pressure >= 1 && genome.shed_pressure <= 100 &&
        genome.late_cow_additions <= 2 && genome.cow_group_override >= -1 &&
        genome.cow_group_override < static_cast<int>(GROUP_START.size());
}

RemapPolicy::RemapPolicy(const Genome& genome) : genome_(genome) {
    if (!valid(genome_)) std::abort();
    build_desired();
}

void RemapPolicy::reset() {
    buy_cursor_ = 0;
    purchase_head_.fill(0);
    purchase_tail_.fill(0);
    for (auto& value : unit_head_) value.fill(0);
    for (auto& value : unit_tail_) value.fill(0);
}

void RemapPolicy::build_desired() {
    const std::array<int, 3> target{
        SOURCE_ANIMALS - genome_.cows - genome_.sheep,
        genome_.cows, genome_.sheep};
    // Source purchase batches, derived once from the strict no-shop tape.
    // Keeping batches intact preserves both the ten-order cap and within-turn
    // liquidity. The group solver changes only batches that reduce the target
    // count error; exact totals are not falsely promised when batch widths make
    // them unreachable.
    constexpr std::array<int, 15> widths{3, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2,
                                         3, 1, 1};
    constexpr std::array<int, 15> source{0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         2, 0, 2};
    std::array<int, widths.size()> choice = source;
    std::array<int, 3> counts{17, 2, 4};
    auto error = [&](const std::array<int, 3>& value) {
        return std::abs(value[0] - target[0]) +
            std::abs(value[1] - target[1]) +
            std::abs(value[2] - target[2]);
    };
    while (true) {
        int best_group = -1;
        int best_species = -1;
        int best_error = error(counts);
        int best_timing = 0;
        for (int group = 0; group < static_cast<int>(widths.size()); ++group)
            for (int species = 0; species < 3; ++species) {
                if (species == choice[group]) continue;
                auto changed = counts;
                changed[choice[group]] -= widths[group];
                changed[species] += widths[group];
                const int changed_error = error(changed);
                const int timing = genome_.animal_order == 0 ? group :
                    genome_.animal_order == 2 ? -group :
                    std::abs(group * SOURCE_ANIMALS /
                             static_cast<int>(widths.size()) -
                             (counts[species] + widths[group]));
                if (changed_error < best_error ||
                    (changed_error == best_error && best_group >= 0 &&
                     timing < best_timing)) {
                    best_group = group;
                    best_species = species;
                    best_error = changed_error;
                    best_timing = timing;
                }
            }
        if (best_group < 0 || best_error >= error(counts)) break;
        counts[choice[best_group]] -= widths[best_group];
        counts[best_species] += widths[best_group];
        choice[best_group] = best_species;
    }
    if (genome_.cow_group_override >= 0)
        choice[genome_.cow_group_override] = 1;
    if (genome_.late_cow_additions >= 1) choice[13] = 1;
    if (genome_.late_cow_additions >= 2) choice[10] = 1;
    int cursor = 0;
    for (int group = 0; group < static_cast<int>(widths.size()); ++group)
        for (int count = 0; count < widths[group]; ++count)
            desired_[cursor++] = static_cast<uint8_t>(
                kag::GOOSE + choice[group]);
    if (cursor != SOURCE_ANIMALS) std::abort();
}

void RemapPolicy::remap_orders(
    const kag::agent::AgentObservation&, kag::Action& action) {
    for (int index = 0; index < action.n_orders; ++index) {
        kag::Order& order = action.orders[index];
        if (order.op == kag::M_BUY_ANIMAL && animal_item(order.item)) {
            // Keep the source order position and width. Existing strategies
            // often sell immediately before a purchase to fund it; splitting
            // and reordering the batch silently destroys that cash contract.
            const int source_species = order.item - kag::GOOSE;
            const uint8_t destination = buy_cursor_ < SOURCE_ANIMALS ?
                desired_[buy_cursor_] : order.item;
            for (int unit = 0; unit < order.n; ++unit) {
                if (purchase_tail_[source_species] >= SOURCE_ANIMALS)
                    std::abort();
                purchase_queue_[source_species]
                               [purchase_tail_[source_species]++] = destination;
            }
            order.item = destination;
            buy_cursor_ = std::min(SOURCE_ANIMALS, buy_cursor_ + order.n);
        } else if (order.op == kag::M_BUY_SEED &&
                   order.item == kag::STRAWBERRY && genome_.tomato_mode) {
            order.item = kag::TOMATO;
        }
    }
}

void RemapPolicy::remap_units(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    std::array<int, 3> available{
        observation.own.shed[kag::GOOSE], observation.own.shed[kag::COW],
        observation.own.shed[kag::SHEEP]};
    for (int unit = 0; unit < action.n_units; ++unit) {
        kag::UnitAction& value = action.units[unit];
        const AnimalUnitContract* exact = nullptr;
        for (const auto& contract : ANIMAL_UNIT_CONTRACTS)
            if (contract.step == observation.step && contract.unit == unit) {
                exact = &contract;
                break;
            }
        if (exact) {
            const int animal = desired_[GROUP_START[exact->group]];
            if (exact->op == ContractOp::Pickup) {
                const int take = std::min<int>(exact->n,
                    available[animal - kag::GOOSE]);
                value = take ? kag::UnitAction{kag::OP_PICKUP,
                    static_cast<uint8_t>(animal), take} : kag::UnitAction{};
                available[animal - kag::GOOSE] -= take;
            } else if (exact->op == ContractOp::Build) {
                value = {static_cast<uint8_t>(animal == kag::GOOSE ?
                    kag::OP_BUILD_COOP : kag::OP_BUILD_PASTURE), 0, 1};
            } else {
                value = {kag::OP_PLACE, static_cast<uint8_t>(animal), 1};
            }
            continue;
        }
        if (value.op == kag::OP_PICKUP && animal_item(value.arg)) {
            const int source_species = value.arg - kag::GOOSE;
            int selected = -1;
            const int wanted = purchase_head_[source_species] <
                    purchase_tail_[source_species] ?
                purchase_queue_[source_species][purchase_head_[source_species]] :
                static_cast<uint8_t>(value.arg);
            if (available[wanted - kag::GOOSE] > 0)
                selected = wanted;
            else
                for (int item = kag::GOOSE; item <= kag::SHEEP; ++item)
                    if (available[item - kag::GOOSE] > 0) {
                        selected = item;
                        break;
                    }
            if (selected < 0) {
                value = {};
                continue;
            }
            const int take = std::min<int>(value.n,
                available[selected - kag::GOOSE]);
            value.arg = static_cast<uint8_t>(selected);
            int queue_take = take;
            if (selected == wanted) {
                queue_take = 0;
                while (queue_take < take &&
                       purchase_head_[source_species] + queue_take <
                           purchase_tail_[source_species] &&
                       purchase_queue_[source_species]
                                      [purchase_head_[source_species] +
                                       queue_take] == selected)
                    ++queue_take;
                if (!queue_take) queue_take = take;
            }
            value.n = queue_take;
            available[selected - kag::GOOSE] -= queue_take;
            purchase_head_[source_species] = static_cast<uint8_t>(
                std::min<int>(purchase_tail_[source_species],
                    purchase_head_[source_species] + queue_take));
            for (int placed = 0; placed < queue_take; ++placed) {
                if (unit_tail_[unit][source_species] >= SOURCE_ANIMALS)
                    std::abort();
                unit_queue_[unit][source_species]
                           [unit_tail_[unit][source_species]++] =
                    static_cast<uint8_t>(selected);
            }
        } else if (value.op == kag::OP_BUILD_COOP ||
                   value.op == kag::OP_BUILD_PASTURE) {
            int source_species = -1;
            if (value.op == kag::OP_BUILD_COOP) {
                source_species = 0;
            } else {
                for (int species : {1, 2})
                    if (unit_head_[unit][species] < unit_tail_[unit][species]) {
                        source_species = species;
                        break;
                    }
            }
            int animal = -1;
            if (source_species >= 0 && unit_head_[unit][source_species] <
                    unit_tail_[unit][source_species])
                animal = unit_queue_[unit][source_species]
                                    [unit_head_[unit][source_species]];
            if (animal < 0) animal = carried_animal(observation, unit);
            if (animal >= 0)
                value.op = animal == kag::GOOSE ? kag::OP_BUILD_COOP :
                    kag::OP_BUILD_PASTURE;
        } else if (value.op == kag::OP_PLACE && animal_item(value.arg)) {
            const int source_species = value.arg - kag::GOOSE;
            int animal = -1;
            if (unit_head_[unit][source_species] <
                    unit_tail_[unit][source_species]) {
                animal = unit_queue_[unit][source_species]
                                    [unit_head_[unit][source_species]++];
            }
            if (animal < 0 || observation.own.inv[unit][animal] <= 0)
                animal = observation.own.inv[unit][value.arg] > 0 ? value.arg :
                    carried_animal(observation, unit);
            if (animal >= 0) value.arg = static_cast<uint8_t>(animal);
        }
        if (genome_.tomato_mode && value.op == kag::OP_PLANT &&
            value.arg == kag::STRAWBERRY)
            value.arg = kag::TOMATO;
    }
    if (genome_.market_mode == 2 && observation.hour == 23) {
        int combined = observation.own.shed_total;
        for (int unit = 0; unit < observation.self().n_units; ++unit)
            for (int item = 0; item < kag::N_ITEMS; ++item)
                combined += observation.own.inv[unit][item];
        if (combined >= genome_.shed_pressure)
            for (int unit = 0; unit < action.n_units; ++unit)
                if (action.units[unit].op == kag::OP_DROP)
                    action.units[unit] = {};
    }
}

void RemapPolicy::schedule_sales(
    const kag::agent::AgentObservation& observation, kag::Action& action) {
    if (!genome_.market_mode) return;
    std::array<int, kag::N_PRODUCTS> existing{};
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order& order = action.orders[index];
        if (order.op == kag::M_SELL && order.item < kag::N_PRODUCTS)
            if (order.item < kag::N_PRODUCTS) existing[order.item] += order.n;
    }

    const bool periodic = observation.step % genome_.sale_period == 0;
    const bool terminal = observation.step >= 714;
    int combined = observation.own.shed_total;
    if (observation.hour == 23)
        for (int unit = 0; unit < observation.self().n_units; ++unit)
            for (int item = 0; item < kag::N_ITEMS; ++item)
                combined += observation.own.inv[unit][item];
    if (observation.hour == 23)
        for (int unit = 0; unit < action.n_units; ++unit) {
            const kag::UnitAction& value = action.units[unit];
            const int x = observation.self().pos_x[unit];
            const int y = observation.self().pos_y[unit];
            const kag::Tile& tile = observation.self().tiles[y][x];
            if (value.op == kag::OP_HARVEST)
                combined += tile.yield_units;
            else if (value.op == kag::OP_COLLECT_FERTILIZER &&
                     tile.has_animal && tile.fertilizer_available)
                ++combined;
        }
    int capacity_sale = std::max(0, combined - genome_.shed_pressure + 1);
    std::array<int, kag::N_PRODUCTS> added{};
    if (genome_.market_mode == 2 && capacity_sale > 0) {
        std::array<int, kag::N_PRODUCTS> order{};
        std::iota(order.begin(), order.end(), 0);
        std::stable_sort(order.begin(), order.end(), [&](int left, int right) {
            return observation.market.prices[left] >
                observation.market.prices[right];
        });
        for (int item : order) {
            const int available = std::max(0,
                static_cast<int>(observation.own.shed[item]) - existing[item]);
            const int quantity = std::min(capacity_sale, available);
            append_order(action, kag::M_SELL, static_cast<uint8_t>(item),
                         quantity);
            added[item] += quantity;
            capacity_sale -= quantity;
            if (!capacity_sale || action.n_orders >= 10) break;
        }
    }
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        int quantity = 0;
        bool target_sale = false;
        if (item == kag::MILK)
            target_sale = observation.market.prices[item] >= genome_.milk_floor;
        else if (item == kag::TOMATO)
            target_sale = observation.market.prices[item] >= genome_.tomato_floor;
        if (terminal) {
            quantity = std::max(0,
                static_cast<int>(observation.own.shed[item]) - existing[item] -
                    added[item]);
        } else if (genome_.market_mode == 1 && periodic && target_sale) {
            quantity = std::max(0,
                static_cast<int>(observation.own.shed[item]) - existing[item]);
        }
        append_order(action, kag::M_SELL, static_cast<uint8_t>(item), quantity);
    }
}

void RemapPolicy::modify(const kag::agent::AgentObservation& observation,
                         kag::Action& action) {
    if (!genome_.enabled) return;
    remap_orders(observation, action);
    remap_units(observation, action);
    schedule_sales(observation, action);
    action.finalize();
}

}  // namespace kag::agents::pizza_125556_robust::pizza
