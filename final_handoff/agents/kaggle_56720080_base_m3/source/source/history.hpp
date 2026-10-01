#pragma once
// Causal opponent market flow, inferred from our own observations only.
// Opponent trades = market inventory change + known town/shop demand - our actual
// fills. Our fills come from the shed change the worker phase does not explain.
#include "world.hpp"
#include <vector>

namespace dc10 {

// Market inventory removed by town and shops at one step (before the step's end).
inline int demand_at(const agent::AgentObservation& o, int item, int step) {
    int d = 0;
    if (step % 4 == 0)
        for (int s = 0; s < o.n_shops; ++s)
            if (SHOP_MASK[o.shops[s]] & (1u << item)) d += SHOP_MULT[o.shops[s]];
    if (step % HOURS == 0 && item != FERTILIZER) d += 1;
    return d;
}

// Opponent output a public observation shows as sellable soon: ripe crops' stored yield and the
// product held on its animals.
inline void visible_supply(const agent::AgentObservation& o, int out[N_PRODUCTS]) {
    std::fill_n(out, N_PRODUCTS, 0);
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = o.opponent().tiles[y][x];
            if (t.has_animal) out[ANIMALS[t.what - GOOSE].product] += t.yield_units;
            else if (t.kind == T_PLANT && o.day - t.planted_day >= CROPS[t.what].first_yield_day) out[t.what] += t.yield_units;
        }
}

class History {
public:
    // The engine copy in infer() lacks our market orders: at hour 23 its night deposit can overflow a shed that our
    // evening sales emptied, and the destroyed units read as opponent sales. exact_shed lifts the copy's shed cap
    // (kaggriculture-38, Sep 26; off = the gated behaviour).
    bool exact_shed = false;
    // Call once per step with the observation before acting and our submitted action.
    void observe(const agent::AgentObservation& o, const Action& submitted) {
        if (has_previous_ && o.step == previous_.step + 1) infer(o);
        previous_ = o;
        previous_action_ = submitted;
        has_previous_ = true;
    }
    // Infers the previous step as soon as this step's observation arrives (before acting); observe() after acting then
    // only records our action. Used by the forecast_tf_exact History (kaggriculture-38: the model was trained with
    // yesterday's hour 23 already inferred at dawn).
    void advance(const agent::AgentObservation& o) {
        if (has_previous_ && o.step == previous_.step + 1) infer(o);
        previous_ = o;
    }
    // Expected opponent net sales (negative = purchases) per hour of day and product,
    // averaged over the last `days` complete days.
    void expected(int today, int days, double out[HOURS][N_PRODUCTS]) const {
        for (int h = 0; h < HOURS; ++h)
            for (int p = 0; p < N_PRODUCTS; ++p) out[h][p] = 0;
        int used = 0;
        for (int d = std::max(0, today - days); d < today; ++d, ++used)
            for (int h = 0; h < HOURS; ++h)
                for (int p = 0; p < N_PRODUCTS; ++p) out[h][p] += flow_at(d * HOURS + h, p);
        if (used)
            for (int h = 0; h < HOURS; ++h)
                for (int p = 0; p < N_PRODUCTS; ++p) out[h][p] /= used;
    }
    int flow_at(int step, int p) const { return step < int(flow_.size()) ? flow_[step][p] : 0; }
    // Opponent finished-product stock it can still sell: publicly observed harvests
    // minus inferred sales (exact from game start except digs and discards).
    const std::array<int, N_PRODUCTS>& opponent_stock() const { return stock_; }
    // Share of what the opponent had available (dawn stock + that day's harvests) that it sold, over
    // the last `days` complete days, shrunk towards `prior` with `weight` pseudo-units.
    // Opponent state recorded at one past dawn and its sales that day (forecast model selection).
    int dawn_visible(int day, int p) const { return day < int(days_.size()) ? days_[day].visible[p] : 0; }
    int dawn_stock(int day, int p) const { return day < int(days_.size()) ? days_[day].dawn_stock[p] : 0; }
    int sold_on(int day, int p) const { return day < int(days_.size()) ? days_[day].sold[p] : 0; }
    // Hourly profile of the opponent's sales of p over all complete days before `today` (shares sum
    // to 1; all zero if it never sold p).
    void hour_profile(int today, int p, double out[HOURS]) const {
        double total = 0;
        for (int h = 0; h < HOURS; ++h) {
            out[h] = 0;
            for (int d = 0; d < today; ++d) out[h] += std::max(0, flow_at(d * HOURS + h, p));
            total += out[h];
        }
        for (int h = 0; h < HOURS; ++h) out[h] = total > 0 ? out[h] / total : 0;
    }
    double sell_through(int today, int p, int days, double prior, double weight = 5) const {
        double sold = 0, available = 0;
        for (int d = std::max(0, today - days); d < today && d < int(days_.size()); ++d)
            sold += days_[d].sold[p], available += days_[d].dawn_stock[p] + days_[d].harvested[p];
        return (sold + prior * weight) / (available + weight);
    }

private:
    void infer(const agent::AgentObservation& next) {
        const auto& o = previous_;
        // Worker phase alone, in an engine copy without market orders.
        Config config;
        if (exact_shed) config.shed_capacity = 1 << 20;
        Sim sim = sim_from_observation(o, config, false);
        Action mine = previous_action_;
        mine.n_orders = 0;
        Action pass;
        pass.finalize();
        o.player == 0 ? sim.step(mine, pass) : sim.step(pass, mine);
        const Farm& after = sim.st.farms[o.player];
        std::array<int, N_PRODUCTS> rival{};
        for (int p = 0; p < N_PRODUCTS; ++p) {
            // At hour 23 the copy also applies tonight's deposit, as the real step does.
            const int fills = next.own.shed[p] - after.shed[p];  // + bought, - sold
            const int own_to_market = -fills;
            const int change = next.market.inventory[p] - o.market.inventory[p];
            rival[p] = change + demand_at(o, p, o.step) - own_to_market;
        }
        if (int(flow_.size()) <= o.step) flow_.resize(o.step + 1);
        flow_[o.step] = rival;
        if (int(days_.size()) <= o.day) days_.resize(o.day + 1);
        DayFlow& day = days_[o.day];
        if (o.hour == 0) {
            day.dawn_stock = stock_;
            int vis[N_PRODUCTS];
            visible_supply(o, vis);
            std::copy_n(vis, N_PRODUCTS, day.visible.begin());
        }
        const auto before = stock_;
        // Harvests on the opponent's board this step.
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) {
                const Tile& a = o.opponent().tiles[y][x];
                const Tile& b = next.opponent().tiles[y][x];
                if (a.yield_units <= 0) continue;
                const bool same = a.kind == b.kind && a.what == b.what && a.has_animal == b.has_animal &&
                                  a.planted_day == b.planted_day;
                if (a.has_animal && same && b.yield_units == 0)
                    stock_[ANIMALS[a.what - GOOSE].product] += a.yield_units;
                else if (a.kind == T_PLANT && o.day - a.planted_day >= CROPS[a.what].first_yield_day &&
                         ((same && b.yield_units == 0 && a.max_lifespan_step < 0) || (!same && b.kind != T_WEED)))
                    stock_[a.what] += a.yield_units;
            }
        for (int p = 0; p < N_PRODUCTS; ++p) {
            day.harvested[p] += stock_[p] - before[p];
            day.sold[p] += std::max(0, rival[p]);
            stock_[p] = std::max(0, stock_[p] - std::max(0, rival[p]));
            if (next.market.prices[p] <= 1) stock_[p] = 0;  // floor sales are invisible
        }
    }
    struct DayFlow { std::array<int, N_PRODUCTS> dawn_stock{}, harvested{}, sold{}, visible{}; };
    std::vector<DayFlow> days_;
    agent::AgentObservation previous_{};
    Action previous_action_{};
    bool has_previous_ = false;
    std::vector<std::array<int, N_PRODUCTS>> flow_;
    std::array<int, N_PRODUCTS> stock_{};
};
}
