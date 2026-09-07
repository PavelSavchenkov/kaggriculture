#include "agent.hpp"

#include <algorithm>
#include <cstdlib>

namespace four_shop_foundry::public_unchanged::deniz_v111 {

#include "tape.inc"

namespace {

constexpr std::array<int, 4> FRONT_RUN_ITEMS = {
    kag::MELON, kag::MILK, kag::STRAWBERRY, kag::WOOL};

kag::UnitAction tape_unit(int step, int wanted_unit) {
    if (step < 0 || step >= TAPE_STEPS) return {};
    int cursor = TAPE_OFFSETS[step];
    const int units = TAPE_DATA[cursor++];
    ++cursor;
    if (wanted_unit < 0 || wanted_unit >= units) return {};
    cursor += 3 * wanted_unit;
    return {static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2]};
}

void tape_action(int step, int units, kag::Action& action) {
    action.clear();
    action.n_units = units;
    for (int unit = 0; unit < units; ++unit) action.units[unit] = {};
    if (step < 0 || step >= TAPE_STEPS) return;
    int cursor = TAPE_OFFSETS[step];
    const int tape_units = TAPE_DATA[cursor++];
    const int tape_orders = TAPE_DATA[cursor++];
    for (int unit = 0; unit < tape_units; ++unit) {
        const kag::UnitAction value{
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2]};
        cursor += 3;
        if (unit < units) action.units[unit] = value;
    }
    action.n_orders = std::min(tape_orders, 10);
    for (int order = 0; order < tape_orders; ++order) {
        const kag::Order value{
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2]};
        cursor += 3;
        if (order < action.n_orders) action.orders[order] = value;
    }
}

int future_quantity(int step, int item) {
    const int future = step + 1;
    if (future < 0 || future >= TAPE_STEPS) return 0;
    int cursor = TAPE_OFFSETS[future];
    const int units = TAPE_DATA[cursor++];
    const int orders = TAPE_DATA[cursor++];
    cursor += 3 * units;
    int result = 0;
    for (int order = 0; order < orders; ++order) {
        const int op = TAPE_DATA[cursor++];
        const int order_item = TAPE_DATA[cursor++];
        const int quantity = TAPE_DATA[cursor++];
        if (op == kag::M_SELL && order_item == item)
            result += std::max(0, quantity);
    }
    return result;
}

int existing_sell(const kag::Action& action, int item) {
    int result = 0;
    for (int order = 0; order < action.n_orders; ++order)
        if (action.orders[order].op == kag::M_SELL &&
            action.orders[order].item == item)
            result += std::max(0, action.orders[order].n);
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

int town_demand_now(const kag::agent::AgentObservation& observation,
                    int item) {
    int demand = observation.step % 24 == 0 ? 1 : 0;
    if (observation.step % 4 != 0) return demand;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (shop < kag::N_SHOPS &&
            (kag::SHOP_MASK[shop] & (uint16_t{1} << item)))
            demand += kag::SHOP_MULT[shop];
    }
    return demand;
}

}

kag::agent::AgentInfo Agent::info() {
    return {"public_deniz_v111_unchanged"};
}

void Agent::reset(const kag::agent::AgentInit& init) {
    player_ = init.player;
    last_step_ = -1;
    due_step_ = -1;
    due_.fill(0);
    repair_start_.fill(-1);
    repair_intended_.fill({});
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    if (observation.player != player_) std::abort();
    const int step = std::clamp(observation.step, 0, TAPE_STEPS - 1);
    if (step == 0 || step < last_step_) {
        due_step_ = -1;
        due_.fill(0);
        repair_start_.fill(-1);
        repair_intended_.fill({});
    }
    last_step_ = step;
    if (due_step_ >= 0 && due_step_ < step) {
        due_step_ = -1;
        due_.fill(0);
    }

    tape_action(step, 1 + observation.own_hand_count(), action);
    for (int unit = action.n_units; unit < kag::MAX_UNITS; ++unit)
        repair_start_[unit] = -1;
    for (int unit = 0; unit < action.n_units; ++unit) {
        if (repair_start_[unit] < 0) continue;
        const int age = step - repair_start_[unit];
        if (age == 1) action.units[unit] = repair_intended_[unit];
        else if (age >= 2 && age <= 9)
            action.units[unit] = tape_unit(step - 1, unit);
        else if (age > 9) repair_start_[unit] = -1;
    }
    for (int unit = 0; unit < action.n_units; ++unit) {
        const uint8_t op = action.units[unit].op;
        if (repair_start_[unit] >= 0 ||
            (op != kag::OP_BUILD_PASTURE && op != kag::OP_PLANT))
            continue;
        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        if (observation.self().tiles[y][x].kind != kag::T_WEED) continue;
        repair_start_[unit] = static_cast<int16_t>(step);
        repair_intended_[unit] = action.units[unit];
        action.units[unit] = {kag::OP_DIG, 0, 1};
    }

    if (front_run_ && due_step_ == step) {
        int kept = 0;
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order order = action.orders[index];
            if (order.op == kag::M_SELL && order.item < kag::N_ITEMS &&
                due_[order.item] > 0) {
                const int reduction = std::min(
                    std::max(0, order.n), due_[order.item]);
                order.n -= reduction;
                due_[order.item] -= reduction;
            }
            if (order.n > 0 || order.op == kag::M_HIRE ||
                order.op == kag::M_BUY_LAND)
                action.orders[kept++] = order;
        }
        action.n_orders = kept;
        due_step_ = -1;
        due_.fill(0);
    }

    std::array<int, kag::N_ITEMS> moved{};
    if (front_run_) for (const int item : FRONT_RUN_ITEMS) {
        const int target = future_quantity(step, item);
        if (target <= 0 || town_demand_now(observation, item) > 0) continue;
        const int stock = std::max<int>(0, observation.own.shed[item]);
        const int reserve = pickup_reserve(action, item) +
            existing_sell(action, item);
        const int quantity = std::min(target, std::max(0, stock - reserve));
        if (quantity <= 0) continue;
        int existing = -1;
        for (int order = 0; order < action.n_orders; ++order)
            if (action.orders[order].op == kag::M_SELL &&
                action.orders[order].item == item) {
                existing = order;
                break;
            }
        if (existing >= 0) action.orders[existing].n += quantity;
        else if (action.n_orders < 10)
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item), quantity};
        else continue;
        moved[item] += quantity;
    }
    if (front_run_ && std::any_of(moved.begin(), moved.end(), [](int value) {
            return value > 0;
        })) {
        due_step_ = step + 1;
        due_ = moved;
    }
    action.finalize();
}

}
