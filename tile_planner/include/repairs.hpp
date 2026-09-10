#pragma once
#include "fast_game_engine/sim.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <vector>

namespace placement {
struct Pending { int step; kag::UnitAction action; };
struct RepairStats {
    int digs = 0, absorbed_passes = 0, skipped_digs = 0, dropped_commands = 0;
    int cash_attempts = 0, advanced_units = 0, canceled_units = 0;
    int guard_checks = 0, guard_rejections = 0, proactive_digs = 0;
};

// Observation-based repair of a caller-owned action plan. This code reads only
// own visible farm state, public market/configuration and that pending plan.
// The same-day queue idea is also used in yhay81/prvsiyan's public Shop Router;
// this independent implementation adds overdue-PASS absorption and accounting.
class PlanRepair {
    int day = -1;
    std::array<std::deque<Pending>, 40> queues;
    std::array<int, kag::N_ITEMS> advanced{};
    std::array<std::deque<kag::UnitAction>, 40> clearing;

    static bool purchase(int op) {
        return op == kag::M_HIRE || op == kag::M_BUY_LAND || op == kag::M_BUY_PRODUCT ||
               op == kag::M_BUY_SEED || op == kag::M_BUY_ANIMAL;
    }

    static kag::Sim own_model(const kag::Farm& farm, const kag::Market& market, const kag::Config& cfg, int step) {
        auto config = cfg; config.starting_money = 0;
        kag::Sim model(config);
        model.st.farms[0] = farm; model.st.market = market;
        model.st.day = step / 24; model.st.hour = 0; model.st.step = step;
        return model;
    }

    static bool missing_purchase(const kag::Sim& model, const kag::Action& action) {
        const auto accepted = model.sanitize_solo_action(0, action);
        for (int s = 0; s < action.n_orders; ++s) if (purchase(action.orders[s].op)) {
            if (accepted.orders[s].op != action.orders[s].op) return true;
            if (action.orders[s].op != kag::M_HIRE && action.orders[s].op != kag::M_BUY_LAND &&
                accepted.orders[s].n != action.orders[s].n) return true;
        }
        return false;
    }

    bool funding_guard(const kag::Farm& farm, const kag::Market& market, const kag::Config& config,
                       const std::vector<kag::Action>& plan, int step, const kag::Action& first,
                       const kag::Action& original, int item, int quantity, bool full_plan, bool profit_guard) const {
        auto cfg = config; cfg.weed_chance = 0;
        auto model = own_model(farm, market, cfg, step);
        model.st.hour = step % 24;
        auto repair = *this;
        auto debt = advanced; debt[item] += quantity;
        const int end = full_plan ? int(plan.size()) : std::min(int(plan.size()), (step / 24 + 2) * 24);
        kag::Action pass; pass.finalize();
        for (int t = step; t < end; ++t) {
            // Stress forecast with no opponent trades, no new weeds and no
            // town demand. It uses only current own/public state and our plan;
            // it is a guard, not a guarantee about the real future market.
            model.st.n_shops = 0;
            auto action = t == step ? first : repair.act(model.st.farms[0], model.st.market, cfg, plan, t, 2);
            if (t > step) for (int s = 0; s < action.n_orders; ++s) {
                auto& order = action.orders[s];
                if (order.op != kag::M_SELL) continue;
                const int cancel = std::min(order.n, debt[order.item]);
                order.n -= cancel; debt[order.item] -= cancel;
                if (!order.n) order = {};
            }
            action.finalize();
            if (model.st.farms[0].n_units < plan[t].n_units) return false;
            const auto outcome = model.diagnose_joint_actions(action, pass).players[0];
            if (outcome.requested_unit_actions != outcome.successful_unit_actions ||
                outcome.requested_order_units != outcome.successful_order_units) return false;
            model.step(action, pass);
        }
        if (!std::all_of(debt.begin(), debt.end(), [](int value) { return value == 0; })) return false;
        if (!profit_guard) return true;
        const double repaired_cash = model.st.farms[0].money;
        model = own_model(farm, market, cfg, step); model.st.hour = step % 24;
        repair = *this; debt = advanced;
        for (int t = step; t < end; ++t) {
            model.st.n_shops = 0;
            auto action = t == step ? original : repair.act(model.st.farms[0], model.st.market, cfg, plan, t, 2);
            if (t > step) for (int s = 0; s < action.n_orders; ++s) {
                auto& order = action.orders[s]; if (order.op != kag::M_SELL) continue;
                const int cancel = std::min(order.n, debt[order.item]); order.n -= cancel; debt[order.item] -= cancel;
                if (!order.n) order = {};
            }
            action.finalize(); model.step(action, pass);
        }
        return repaired_cash >= model.st.farms[0].money;
    }

    void clear_with_idle_worker(const kag::Farm& farm, const std::vector<kag::Action>& plan, int step, kag::Action& result) {
        const int end = std::min(int(plan.size()), (step / 24 + 1) * 24);
        std::array<int, 100> planned_clear; planned_clear.fill(end);
        for (int v = 0; v < farm.n_units; ++v) {
            if (!queues[v].empty()) continue;
            int x = farm.pos_x[v], y = farm.pos_y[v];
            for (int t = step; t < end && v < plan[t].n_units; ++t) {
                const auto a = plan[t].units[v];
                x += (a.op == kag::OP_EAST) - (a.op == kag::OP_WEST);
                y += (a.op == kag::OP_SOUTH) - (a.op == kag::OP_NORTH);
                if (x < 0 || x >= 10 || y < 0 || y >= 10) break;
                if (a.op == kag::OP_DIG) planned_clear[y * 10 + x] = std::min(planned_clear[y * 10 + x], t);
            }
        }
        for (int u = 0; u < farm.n_units; ++u) {
            if (!queues[u].empty() || !clearing[u].empty() || result.units[u].op != kag::OP_PASS) continue;
            int idle = 1;
            while (step + idle < end && u < plan[step + idle].n_units && plan[step + idle].units[u].op == kag::OP_PASS) ++idle;
            int chosen = -1, best_distance = 100;
            for (int v = 0; v < farm.n_units; ++v) {
                if (!queues[v].empty() || !clearing[v].empty()) continue;
                int x = farm.pos_x[v], y = farm.pos_y[v];
                for (int t = step; t < end; ++t) {
                    if (v >= plan[t].n_units) break;
                    const auto a = plan[t].units[v];
                    x += (a.op == kag::OP_EAST) - (a.op == kag::OP_WEST);
                    y += (a.op == kag::OP_SOUTH) - (a.op == kag::OP_NORTH);
                    if (x < 0 || x >= 10 || y < 0 || y >= 10) break;
                    if (a.op != kag::OP_PLANT && a.op != kag::OP_BUILD_COOP && a.op != kag::OP_BUILD_PASTURE) continue;
                    if (farm.tiles[y][x].kind != kag::T_WEED || planned_clear[y * 10 + x] <= t) continue;
                    const int distance = std::abs(x - farm.pos_x[u]) + std::abs(y - farm.pos_y[u]);
                    bool changes_spawn = false;
                    for (int h = step; h < std::min(end, step + 2 * distance); ++h)
                        for (int s = 0; s < plan[h].n_orders; ++s) changes_spawn |= plan[h].orders[s].op == kag::M_HIRE;
                    if (changes_spawn) continue;
                    if (2 * distance + 1 <= idle && step + distance < t && distance < best_distance) {
                        chosen = y * 10 + x; best_distance = distance;
                    }
                }
            }
            if (chosen < 0) continue;
            const int origin_x = farm.pos_x[u], origin_y = farm.pos_y[u];
            int x = origin_x, y = origin_y;
            auto walk = [&](int to_x, int to_y) {
                while (x != to_x) { const int delta = x < to_x ? 1 : -1; clearing[u].push_back({uint8_t(delta > 0 ? kag::OP_EAST : kag::OP_WEST), 0, 0}); x += delta; }
                while (y != to_y) { const int delta = y < to_y ? 1 : -1; clearing[u].push_back({uint8_t(delta > 0 ? kag::OP_SOUTH : kag::OP_NORTH), 0, 0}); y += delta; }
            };
            walk(chosen % 10, chosen / 10); clearing[u].push_back({kag::OP_DIG, 0, 0}); walk(origin_x, origin_y);
            result.units[u] = clearing[u].front(); clearing[u].pop_front(); ++stats.proactive_digs;
            // One in-flight clearing route at a time avoids duplicate targets.
            break;
        }
    }

public:
    RepairStats stats;
    void finish_day() {
        for (auto& queue : queues) {
            for (const auto& pending : queue) stats.dropped_commands += pending.action.op != kag::OP_PASS;
            queue.clear();
        }
        for (auto& route : clearing) { stats.dropped_commands += route.size(); route.clear(); }
    }

    kag::Action act(const kag::Farm& farm, const kag::Market& market, const kag::Config& config,
                    const std::vector<kag::Action>& plan, int step, int mode) {
        auto result = plan[step];
        if (!mode) return result;
        if (step / 24 != day) { finish_day(); day = step / 24; }
        result.n_units = farm.n_units;
        for (int u = 0; u < farm.n_units; ++u) {
            auto& queue = queues[u];
            queue.push_back({step, u < plan[step].n_units ? plan[step].units[u] : kag::UnitAction{}});
            if (mode >= 2) while (queue.size() > 1 && queue.front().step < step && queue.front().action.op == kag::OP_PASS) {
                queue.pop_front(); ++stats.absorbed_passes;
            }
            const auto action = queue.front().action;
            const auto& tile = farm.tiles[farm.pos_y[u]][farm.pos_x[u]];
            const bool blocked = tile.kind == kag::T_WEED && (action.op == kag::OP_PLANT ||
                action.op == kag::OP_BUILD_COOP || action.op == kag::OP_BUILD_PASTURE);
            if (blocked) { result.units[u] = {kag::OP_DIG, 0, 0}; ++stats.digs; }
            else {
                result.units[u] = action; queue.pop_front();
                if (mode >= 2 && action.op == kag::OP_DIG && tile.kind == kag::T_EMPTY) {
                    result.units[u] = {}; ++stats.skipped_digs;
                }
            }
        }
        if (mode == 6) {
            const bool active = std::any_of(clearing.begin(), clearing.end(), [](const auto& route) { return !route.empty(); });
            if (active) {
                for (int u = 0; u < farm.n_units; ++u) if (!clearing[u].empty()) {
                    result.units[u] = clearing[u].front(); clearing[u].pop_front();
                }
            } else clear_with_idle_worker(farm, plan, step, result);
        }
        if (mode < 3 || mode == 6) { result.finalize(); return result; }
        for (int s = 0; s < result.n_orders; ++s) {
            auto& order = result.orders[s];
            if (order.op != kag::M_SELL) continue;
            const int cancel = std::min(order.n, advanced[order.item]);
            order.n -= cancel; advanced[order.item] -= cancel; stats.canceled_units += cancel;
            if (!order.n) order = {};
        }
        result.finalize();
        bool has_purchase = false;
        for (int s = 0; s < result.n_orders; ++s) has_purchase |= purchase(result.orders[s].op);
        if (!has_purchase) return result;
        const auto model = own_model(farm, market, config, step);
        if (!missing_purchase(model, result)) return result;
        ++stats.cash_attempts;
        std::array<int, kag::N_ITEMS> future_sales{}, reserve{}, balance{};
        for (int t = step + 1; t < int(plan.size()); ++t)
            for (int s = 0; s < plan[t].n_orders; ++s) if (plan[t].orders[s].op == kag::M_SELL)
                future_sales[plan[t].orders[s].item] += plan[t].orders[s].n;
        for (int i = 0; i < kag::N_ITEMS; ++i) future_sales[i] = std::max(0, future_sales[i] - advanced[i]);
        // Protect planned input pickups until replenishment. Market buys occur
        // after that phase's worker pickups, matching the engine's ordering.
        for (const auto& queue : queues) for (const auto& pending : queue)
            if (pending.action.op == kag::OP_PICKUP) balance[pending.action.arg] += pending.action.n;
        reserve = balance;
        for (int t = step + 1; t < std::min(int(plan.size()), (day + 1) * 24); ++t) {
            for (int u = 0; u < plan[t].n_units; ++u) if (plan[t].units[u].op == kag::OP_PICKUP) {
                const auto& action = plan[t].units[u]; balance[action.arg] += action.n;
                reserve[action.arg] = std::max(reserve[action.arg], balance[action.arg]);
            }
            for (int s = 0; s < plan[t].n_orders; ++s) if (plan[t].orders[s].op == kag::M_BUY_PRODUCT)
                balance[plan[t].orders[s].item] -= plan[t].orders[s].n;
        }
        auto after_workers = model; auto workers = result; workers.n_orders = 0; workers.finalize();
        kag::Action pass; pass.finalize(); after_workers.step(workers, pass);
        const auto& shed = after_workers.st.farms[0].shed;
        std::vector<int> items;
        for (int item = 0; item < kag::N_PRODUCTS; ++item) if (future_sales[item] && shed[item] > reserve[item]) items.push_back(item);
        std::stable_sort(items.begin(), items.end(), [&](int a, int b) { return market.prices[a] > market.prices[b]; });
        for (int item : items) {
            const int limit = std::min(future_sales[item], int(shed[item]) - reserve[item]);
            for (int quantity = 1; quantity <= limit; ++quantity) {
                auto candidate = result;
                int first_purchase = 10;
                for (int s = 0; s < candidate.n_orders; ++s) if (purchase(candidate.orders[s].op)) { first_purchase = s; break; }
                int slot = -1;
                for (int s = 0; s < first_purchase; ++s) if (candidate.orders[s].op == kag::M_NONE) { slot = s; break; }
                if (slot < 0) {
                    if (candidate.n_orders >= 10) break;
                    std::move_backward(candidate.orders, candidate.orders + candidate.n_orders, candidate.orders + candidate.n_orders + 1);
                    ++candidate.n_orders; slot = 0;
                }
                candidate.orders[slot] = {kag::M_SELL, uint8_t(item), quantity}; candidate.finalize();
                if (missing_purchase(model, candidate)) continue;
                if (mode >= 4) {
                    ++stats.guard_checks;
                    if (!funding_guard(farm, market, config, plan, step, candidate, result, item, quantity, mode >= 5, mode == 7)) {
                        ++stats.guard_rejections;
                        // Extra liquidation is not justified by a failed
                        // forecast for this item's minimum funding amount.
                        break;
                    }
                }
                advanced[item] += quantity; stats.advanced_units += quantity;
                return candidate;
            }
        }
        return result;
    }
};
}
