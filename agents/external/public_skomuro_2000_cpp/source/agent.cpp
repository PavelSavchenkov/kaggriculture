#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace four_shop_search::public_skomuro_2000_cpp {

namespace {

#include "tape.inc"

constexpr std::array<int, 4> PREMIUM = {
    kag::MELON, kag::STRAWBERRY, kag::MILK, kag::WOOL,
};
constexpr std::array<std::array<int, 2>, 4> SHED_TILES = {{
    {4, 4}, {5, 4}, {4, 5}, {5, 5},
}};

bool is_premium(int item) {
    return std::find(PREMIUM.begin(), PREMIUM.end(), item) != PREMIUM.end();
}

template<class T, std::size_t N, class Before>
void stable_insertion_sort(std::array<T, N>& values, int count,
                           Before before) {
    for (int index = 1; index < count; ++index) {
        const T value = values[index];
        int position = index;
        while (position > 0 && before(value, values[position - 1])) {
            values[position] = values[position - 1];
            --position;
        }
        values[position] = value;
    }
}

bool is_move(int op) {
    return op >= kag::OP_NORTH && op <= kag::OP_WEST;
}

bool is_terminal_replaceable(int op) {
    return is_move(op) || op == kag::OP_PASS || op == kag::OP_DROP ||
        op == kag::OP_PLACE || op == kag::OP_WATER || op == kag::OP_CARE ||
        op == kag::OP_FEED;
}

bool is_tile_op(int op) {
    return op == kag::OP_PLANT || op == kag::OP_BUILD_PASTURE ||
        op == kag::OP_BUILD_COOP || op == kag::OP_PLACE;
}

kag::UnitAction pass_action() {
    return {};
}

kag::UnitAction tape_unit(int step, int actor) {
    if (step < 0 || step >= TAPE_STEPS) return pass_action();
    int cursor = TAPE_OFFSETS[step];
    const int units = TAPE_DATA[cursor++];
    ++cursor;
    if (actor >= units) return pass_action();
    cursor += actor * 3;
    return {
        static_cast<uint8_t>(TAPE_DATA[cursor]),
        static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
        TAPE_DATA[cursor + 2],
    };
}

void tape_orders(int step, kag::Action& action) {
    action.n_orders = 0;
    if (step < 0 || step >= TAPE_STEPS) return;
    int cursor = TAPE_OFFSETS[step];
    const int units = TAPE_DATA[cursor++];
    const int orders = TAPE_DATA[cursor++];
    cursor += units * 3;
    for (int index = 0; index < orders; ++index) {
        action.orders[action.n_orders++] = {
            static_cast<uint8_t>(TAPE_DATA[cursor]),
            static_cast<uint8_t>(TAPE_DATA[cursor + 1]),
            TAPE_DATA[cursor + 2],
        };
        cursor += 3;
    }
}

int shed_distance(int x, int y) {
    int best = 100;
    for (const auto& target : SHED_TILES)
        best = std::min(best, std::abs(x - target[0]) +
            std::abs(y - target[1]));
    return best;
}

kag::UnitAction toward_shed(int x, int y) {
    int best = 100;
    int tx = 4;
    int ty = 4;
    for (const auto& target : SHED_TILES) {
        const int distance = std::abs(x - target[0]) +
            std::abs(y - target[1]);
        if (distance >= best) continue;
        best = distance;
        tx = target[0];
        ty = target[1];
    }
    if (x < tx) return {kag::OP_EAST, 0, 1};
    if (x > tx) return {kag::OP_WEST, 0, 1};
    if (y < ty) return {kag::OP_SOUTH, 0, 1};
    if (y > ty) return {kag::OP_NORTH, 0, 1};
    return {kag::OP_DROP, 0, 1};
}

int carried_value(const kag::agent::AgentObservation& observation, int unit) {
    int value = 0;
    for (int item : PREMIUM)
        value += observation.own.inv[unit][item] *
            observation.market.prices[item];
    return value;
}

bool carrying(const kag::agent::AgentObservation& observation, int unit) {
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (observation.own.inv[unit][item] > 0) return true;
    return false;
}

int wool_edge(const kag::agent::AgentObservation& observation) {
    int edge = 0;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if (shop == kag::SHOP_YARN_STORE) edge += 12;
        if (shop == kag::SHOP_PIZZA_SHOP ||
            shop == kag::SHOP_ICE_CREAM_SHOP ||
            shop == kag::SHOP_SMOOTHIE_SHOP)
            edge -= 6;
    }
    return edge;
}

void erase_order(kag::Action& action, int index) {
    for (int move = index; move + 1 < action.n_orders; ++move)
        action.orders[move] = action.orders[move + 1];
    --action.n_orders;
}

int scheduled_sale(const kag::Action& action, int item) {
    int total = 0;
    for (int index = 0; index < action.n_orders; ++index)
        if (action.orders[index].op == kag::M_SELL &&
            action.orders[index].item == item)
            total += action.orders[index].n;
    return total;
}

int impact(const kag::agent::AgentObservation& observation,
           const kag::Order& order) {
    const int item = order.item;
    const int inventory = observation.market.inventory[item];
    return order.n * std::max(0, kag::market_price(item, inventory) -
        kag::market_price(item, inventory + order.n));
}

void premium_first(const kag::agent::AgentObservation& observation,
                   kag::Action& action) {
    std::array<kag::Order, 16> premium{};
    std::array<kag::Order, 16> other{};
    int premium_count = 0;
    int other_count = 0;
    for (int index = 0; index < action.n_orders; ++index) {
        const kag::Order order = action.orders[index];
        if (order.op == kag::M_SELL && is_premium(order.item))
            premium[premium_count++] = order;
        else
            other[other_count++] = order;
    }
    stable_insertion_sort(premium, premium_count,
        [&](const kag::Order& first, const kag::Order& second) {
            return impact(observation, first) > impact(observation, second);
        });
    action.n_orders = 0;
    for (int index = 0; index < premium_count; ++index)
        action.orders[action.n_orders++] = premium[index];
    for (int index = 0; index < other_count; ++index)
        action.orders[action.n_orders++] = other[index];
}

}  // namespace

kag::agent::AgentInfo Agent::info() {
    return {"public_skomuro_2000_cpp"};
}

void Agent::reset(const kag::agent::AgentInit&) {
    last_step_ = -1;
    converted_ = 0;
    last_wool_sell_ = -99;
    delay_.fill(0);
    for (auto& row : debt_) row.fill(0);
}

void Agent::act(const kag::agent::AgentObservation& observation,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    const int step = observation.step;
    if (step == 0 || step < last_step_) reset({});
    last_step_ = step;
    if (observation.hour == 0) delay_.fill(0);

    action.clear();
    action.n_units = observation.self().n_units;
    for (int unit = 0; unit < action.n_units; ++unit) {
        int lag = delay_[unit];
        kag::UnitAction command = tape_unit(step - lag, unit);
        if (lag > 0 && command.op == kag::OP_PASS) {
            --delay_[unit];
            command = tape_unit(step - lag + 1, unit);
        }

        if (command.arg == kag::COW) {
            if (command.op == kag::OP_PICKUP &&
                observation.own.shed[kag::COW] == 0 &&
                observation.own.shed[kag::SHEEP] > 0)
                command.arg = kag::SHEEP;
            if (command.op == kag::OP_PLACE &&
                observation.own.inv[unit][kag::COW] == 0 &&
                observation.own.inv[unit][kag::SHEEP] > 0)
                command.arg = kag::SHEEP;
        }

        const int x = observation.self().pos_x[unit];
        const int y = observation.self().pos_y[unit];
        if (is_tile_op(command.op) &&
            observation.self().tiles[y][x].kind == kag::T_WEED) {
            command = {kag::OP_DIG, 0, 1};
            ++delay_[unit];
        }

        if (step >= 120 && step <= 679 &&
            (command.op == kag::OP_PASS || is_move(command.op)) &&
            carried_value(observation, unit) >= 2000) {
            const int distance = shed_distance(x, y);
            if (distance == 0)
                command = {kag::OP_DROP, 0, 1};
            else if (distance <= 1)
                command = toward_shed(x, y);
        }
        action.units[unit] = command;
    }

    tape_orders(step, action);

    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        int owed = debt_[step][item];
        for (int index = 0; index < action.n_orders && owed > 0;) {
            kag::Order& order = action.orders[index];
            if (order.op != kag::M_SELL || order.item != item) {
                ++index;
                continue;
            }
            const int take = std::min(order.n, owed);
            order.n -= take;
            owed -= take;
            if (order.n == 0)
                erase_order(action, index);
            else
                ++index;
        }
        debt_[step][item] = 0;
    }

    if (step >= 144 && wool_edge(observation) > 0 && converted_ < 4 &&
        observation.self().money >= 1100) {
        std::array<kag::Order, 16> extra{};
        int extra_count = 0;
        for (int index = 0; index < action.n_orders; ++index) {
            kag::Order& order = action.orders[index];
            if (order.op != kag::M_BUY_ANIMAL || order.item != kag::COW)
                continue;
            const int original = order.n;
            const int quantity = std::min(original, 4 - converted_);
            if (quantity <= 0) break;
            order.item = kag::SHEEP;
            order.n = quantity;
            if (original > quantity)
                extra[extra_count++] = {
                    kag::M_BUY_ANIMAL, kag::COW, original - quantity,
                };
            converted_ += quantity;
        }
        for (int index = 0; index < extra_count && action.n_orders < 16;
             ++index)
            action.orders[action.n_orders++] = extra[index];
    }

    if (converted_ > 0 && step - last_wool_sell_ >= 6) {
        const int surplus = observation.own.shed[kag::WOOL] -
            scheduled_sale(action, kag::WOOL);
        const int gate = step <= 480 ? 170 : step <= 600 ? 120 :
            step <= 672 ? 80 : 1;
        if (surplus > 0 && action.n_orders < 10 &&
            observation.market.prices[kag::WOOL] >= gate) {
            action.orders[action.n_orders++] = {
                kag::M_SELL, kag::WOOL, std::min(surplus, 16),
            };
            last_wool_sell_ = step;
        }
    }

    if (step >= 120 && step <= 714) {
        kag::Action next;
        tape_orders(step + 1, next);
        for (int index = 0; index < next.n_orders; ++index) {
            const kag::Order order = next.orders[index];
            if (order.op != kag::M_SELL ||
                (order.item != kag::WHEAT && order.item != kag::FERTILIZER))
                continue;
            const int cap = order.item == kag::WHEAT ? 10 : 5;
            int quantity = std::min({order.n, cap,
                static_cast<int>(observation.own.shed[order.item])});
            if (quantity <= 0 || action.n_orders >= 10 ||
                kag::market_price(order.item,
                    observation.market.inventory[order.item]) <= 2)
                continue;
            quantity = std::min(quantity,
                static_cast<int>(observation.own.shed[order.item]) -
                scheduled_sale(action, order.item));
            if (quantity <= 0) continue;
            action.orders[action.n_orders++] = {
                kag::M_SELL, order.item, quantity,
            };
            debt_[step + 1][order.item] += quantity;
        }
    }

    if (step >= 700 && step <= 718) {
        std::array<int, kag::N_PRODUCTS> items{};
        for (int item = 0; item < kag::N_PRODUCTS; ++item) items[item] = item;
        stable_insertion_sort(items, kag::N_PRODUCTS,
            [&](int first, int second) {
            return observation.market.prices[first] *
                observation.own.shed[first] >
                observation.market.prices[second] *
                observation.own.shed[second];
        });
        const int left = std::max(1, 718 - step);
        for (int item : items) {
            const int quantity = observation.own.shed[item];
            if (quantity <= 0 || scheduled_sale(action, item) > 0) continue;
            if (action.n_orders >= 10) break;
            action.orders[action.n_orders++] = {
                kag::M_SELL, static_cast<uint8_t>(item),
                std::max(1, quantity / left),
            };
        }
    }

    if (step >= 714) {
        for (int unit = 0; unit < action.n_units; ++unit) {
            if (!carrying(observation, unit) ||
                !is_terminal_replaceable(action.units[unit].op))
                continue;
            action.units[unit] = toward_shed(
                observation.self().pos_x[unit],
                observation.self().pos_y[unit]);
        }
        if (step == 718) {
            std::array<int, kag::N_PRODUCTS> pending{};
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                pending[item] = observation.own.shed[item];
            for (int unit = 0; unit < action.n_units; ++unit) {
                const int x = observation.self().pos_x[unit];
                const int y = observation.self().pos_y[unit];
                if (shed_distance(x, y) != 0) continue;
                for (int item = 0; item < kag::N_PRODUCTS; ++item)
                    pending[item] += observation.own.inv[unit][item];
            }
            int write = 0;
            for (int index = 0; index < action.n_orders; ++index)
                if (action.orders[index].op == kag::M_SELL)
                    action.orders[write++] = action.orders[index];
            action.n_orders = write;
            std::array<int, kag::N_PRODUCTS> items{};
            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                items[item] = item;
            stable_insertion_sort(items, kag::N_PRODUCTS,
                [&](int first, int second) {
                return observation.market.prices[first] * pending[first] >
                    observation.market.prices[second] * pending[second];
            });
            for (int item : items) {
                if (pending[item] <= 0 || scheduled_sale(action, item) > 0)
                    continue;
                if (action.n_orders >= 10) break;
                action.orders[action.n_orders++] = {
                    kag::M_SELL, static_cast<uint8_t>(item), pending[item],
                };
            }
        }
    }

    if (step == 0) {
        std::array<kag::Order, 16> ordered{};
        int write = 0;
        for (int index = 0; index < action.n_orders; ++index)
            if (action.orders[index].op == kag::M_BUY_PRODUCT &&
                action.orders[index].item == kag::WHEAT)
                ordered[write++] = action.orders[index];
        for (int index = 0; index < action.n_orders; ++index)
            if (action.orders[index].op != kag::M_BUY_PRODUCT ||
                action.orders[index].item != kag::WHEAT)
                ordered[write++] = action.orders[index];
        std::copy_n(ordered.begin(), action.n_orders, action.orders);
    }
    if (step > 0) premium_first(observation, action);
    if (action.n_orders > 10) action.n_orders = 10;
    action.finalize();
}

}  // namespace four_shop_search::public_skomuro_2000_cpp
