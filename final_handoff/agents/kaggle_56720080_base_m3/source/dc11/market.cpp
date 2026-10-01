#include "dc11/market.hpp"
#include "source/opp_prior.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include <limits>
#include <random>
#include <vector>

namespace dc11 {

double sale_value(int item, int inventory, int units) {
    double v = 0;
    for (int k = 0; k < units; ++k) v += market_price(item, inventory + k);
    return v;
}

namespace {
// Fitted opponent model (frozen FORECAST_FIT): coefficients of trailing sales, visible output,
// inferred shed stock, sell-through x supply, day / 30 and 1.
constexpr double FIT[N_PRODUCTS][6] = {
    {0, 0, 0, 0, 0, 0},
    {-0.0244, -0.0077, 0.9766, 0.0409, 0.0228, -0.0084},
    {0.0194, 0.0041, 0.9987, -0.0005, -0.0385, 0.0131},
    {0.7552, 0.2545, 0.7149, -0.2062, 1.3505, -0.3041},
    {-0.0006, 0.7359, 0.6489, 0.4380, 0.0008, -0.0040},
    {-0.0533, -0.0282, 0.9668, 0.0857, 0.1577, -0.0302},
    {0.2635, 0.2251, 0.5319, -0.1765, 6.3437, -0.7765},
    {-0.0125, 0.3442, 0.5496, -0.0131, 4.9130, -0.6515},
    {0, 0, 0, 0, 0, 0},
};

double fit_value(int p, double trailing, double visible, double stock, double through, int day) {
    const auto& c = FIT[p];
    return std::max(0.0, c[0] * trailing + c[1] * visible + c[2] * stock + c[3] * through * (visible + stock) +
                             c[4] * day / 30.0 + c[5]);
}
// blend: weight of visible supply (melons: visible only).
double blend_value(int p, double trailing, double visible, double blend) {
    return p == MELON ? visible : (1 - blend) * trailing + blend * visible;
}
bool forecast_product(int p) { return p != WHEAT && p != FERTILIZER; }
}

void first_sales(const agent::AgentObservation& o, double rival[HOURS][N_PRODUCTS]) {
    if (o.day >= LAST_DAY) return;
    int visible[N_PRODUCTS];
    visible_supply(o, visible);
    const int bucket = o.day <= 9 ? 0 : o.day <= 14 ? 1 : o.day <= 22 ? 2 : 3;
    for (const int p : {int(MILK), int(WOOL)}) {
        double total = 0;
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
        if (total > 0 || visible[p] <= 0) continue;
        for (int h = 0; h < HOURS; ++h) rival[h][p] = visible[p] * OPP_PRIOR[p][bucket][0][h];
    }
}

void forecast(const agent::AgentObservation& o, const History& history, double rival[HOURS][N_PRODUCTS], int first_full,
              double blend) {
    history.expected(o.day, 3, rival);
    if (o.day >= LAST_DAY) return;
    if (first_full) first_sales(o, rival);
    int visible[N_PRODUCTS];
    visible_supply(o, visible);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (!forecast_product(p)) continue;
        // Which model predicted this opponent better over the last 3 days.
        double error_blend = 0, error_fit = 0;
        for (int k = std::max(1, o.day - 3); k < o.day; ++k) {
            double past[HOURS][N_PRODUCTS];
            history.expected(k, 3, past);
            double trailing = 0;
            for (int h = 0; h < HOURS; ++h) trailing += std::max(0.0, past[h][p]);
            const double vis = history.dawn_visible(k, p), stock = history.dawn_stock(k, p);
            error_blend += std::abs(blend_value(p, trailing, vis, blend) - history.sold_on(k, p));
            error_fit += std::abs(fit_value(p, trailing, vis, stock, history.sell_through(k, p, 3, 0.6), k) - history.sold_on(k, p));
        }
        double total = 0;
        for (int h = 0; h < HOURS; ++h) total += std::max(0.0, rival[h][p]);
        const double stock = history.opponent_stock()[p];
        const double target = error_fit < error_blend
                                  ? fit_value(p, total, visible[p], stock, history.sell_through(o.day, p, 3, 0.6), o.day)
                                  : blend_value(p, total, visible[p], blend);
        for (int h = 0; h < HOURS; ++h) {
            const double share = total > 0 ? std::max(0.0, rival[h][p]) / total : (h >= 6 && h <= 12 ? 1.0 / 7 : 0.0);
            rival[h][p] = rival[h][p] < 0 ? rival[h][p] : target * share;
        }
    }
}

void stress_forecast(const agent::AgentObservation& dawn, const History& history, double rival[HOURS][N_PRODUCTS], int stress_hour) {
    history.expected(dawn.day, 3, rival);
    int visible[N_PRODUCTS];
    visible_supply(dawn, visible);
    const int hour = std::min(stress_hour, last_hour(dawn.day));
    for (int p = 0; p < N_PRODUCTS; ++p) rival[hour][p] += visible[p];
}

namespace {
int floor_inventory(int p) {  // the lowest market inventory quoted at $1 (sales there add no inventory)
    static const std::array<int, N_PRODUCTS> limits = [] {
        std::array<int, N_PRODUCTS> a{};
        for (int q = 0; q < N_PRODUCTS; ++q) {
            int lo = -100000, hi = 100000;
            while (lo + 1 < hi) {
                const int mid = (lo + hi) / 2;
                (market_price(q, mid) > 1 ? lo : hi) = mid;
            }
            a[q] = hi;
        }
        return a;
    }();
    return limits[p];
}
}

// The same DP with inventory capped at the floor: state (hour, units sold, units that added no
// inventory). Memoized over reachable states under a fixed operation bound.
bool ProductSale::solve_floor(double charge, int& first, int& left) {
    const int ceiling = floor_inventory(product);
    int rival_total = 0, future_total = 0;
    for (int k = 0; k < remaining; ++k) rival_total += rival_units[k];
    for (int k = 0; k < HOURS / 2; ++k) future_total += future_rival[k];
    if (inv0 + total + rival_total + (future_floor ? future_total : 0) < ceiling) return false;
    const int max_lost = total + rival_total, width = total + 1, depth = max_lost + 1;
    const long long states = (remaining + 1LL) * width * depth;
    if (states > 400000) return false;
    std::vector<double> memo(states, std::numeric_limits<double>::quiet_NaN());
    std::vector<int16_t> best_q(states, 0);
    long long operations = 0;
    constexpr long long operation_limit = 3000000;
    auto index = [&](int k, int s, int lost) { return (static_cast<long long>(k) * width + s) * depth + lost; };
    auto hold_value = [&](int s, int lost) {
        const int l = total - s;
        if (terminal || !l) return -charge * l;
        int inventory = inv0 + s - cum_demand[remaining] - lost;
        if (future_floor)
            for (int j = 0; j < HOURS / 2; ++j) {
                if (inventory < ceiling) inventory = std::min(ceiling, inventory + future_rival[j]);
                inventory -= step_demand[remaining + j];
            }
        else inventory -= cum_demand[remaining + HOURS / 2] - cum_demand[remaining];
        return hold_discount * sale_value(product, inventory + hold_supply, l) - charge * l;
    };
    auto solve_at = [&](auto&& self, int k, int s, int lost) -> double {
        const long long id = index(k, s, lost);
        if (!std::isnan(memo[id])) return memo[id];
        if (k == remaining) return memo[id] = hold_value(s, lost);
        const int inventory = inv0 + s - cum_demand[k] - lost, r = rival_units[k];
        auto rival_cost = [&](int q) {
            if (!rival_weight || !r) return 0.0;
            double v = 0;
            for (int j = 0; j < r; ++j) v += market_price(product, inventory + j + (interleave ? std::min(j, q) : q));
            return rival_weight * v;
        };
        auto next = [&](int q) {
            const int added = std::clamp(ceiling - inventory, 0, q + r);
            const double later = self(self, k + 1, s + q, std::min(max_lost, lost + q + r - added));
            return wait_cost > 0 ? (1 - wait_cost) * later : later;
        };
        double best = next(0) - rival_cost(0), gained = 0;
        int chosen = 0;
        if (keep_first && k == 0 && s == 0 && lost == 0) first_values.assign(1, best);
        const int top = lot_cap > 0 && k + 1 < remaining ? std::min(avail[k] - s, lot_cap) : avail[k] - s;
        for (int q = 1; q <= top; ++q) {
            gained += market_price(product, inventory + q - 1 + (interleave ? std::min(q - 1, r) : 0));
            const double v = gained * (1 + bonus[k]) + next(q) - rival_cost(q);
            if (keep_first && k == 0 && s == 0 && lost == 0) first_values.push_back(v);
            if (v > best || (tie_now && v >= best - 1e-6)) best = v, chosen = q;
        }
        operations += (avail[k] - s + 1) * (r + 1);
        best_q[id] = int16_t(chosen);
        return memo[id] = best;
    };
    // Bound the work before committing: a cheap probe of the reachable size is the solve itself, so
    // cap it and fall back (never use a partially solved policy).
    value = solve_at(solve_at, 0, 0, 0);
    if (operations > operation_limit) return false;
    int s = 0, lost = 0;
    for (int k = 0; k < remaining; ++k) {
        const int q = best_q[index(k, s, lost)];
        if (!k) first = q;
        const int inventory = inv0 + s - cum_demand[k] - lost, r = rival_units[k];
        revenue[k] = sale_value(product, inventory, q);
        lost = std::min(max_lost, lost + q + r - std::clamp(ceiling - inventory, 0, q + r));
        s += q;
    }
    left = total - s;
    return true;
}

int ProductSale::solve(double charge, int& first) {
    if (int left = 0; floor && solve_floor(charge, first, left)) return left;
    const int width = total + 1;
    std::vector<double> next(width), cur(width);
    std::vector<int> choice(remaining * width, 0);
    for (int s = 0; s <= total; ++s) {
        const int left = total - s;
        const int hold_inventory = inv0 + s - cum_demand[remaining + HOURS / 2] + hold_supply;
        next[s] = terminal || !left ? 0 : hold_discount * sale_value(product, hold_inventory, left);
        next[s] -= charge * left;
    }
    for (int k = remaining - 1; k >= 0; --k) {
        for (int s = 0; s <= total; ++s) {
            const int inventory = inv0 + s - cum_demand[k];
            const int r = rival_units[k];
            auto rival_cost = [&](int q) {
                if (!rival_weight || !r) return 0.0;
                if (!interleave) return rival_weight * sale_value(product, inventory + q, r);
                double v = 0;
                for (int j = 0; j < r; ++j) v += market_price(product, inventory + j + std::min(j, q));
                return rival_weight * v;
            };
            auto later = [&](int t) { return wait_cost > 0 ? (1 - wait_cost) * next[t] : next[t]; };
            double best = later(s) - rival_cost(0), gained = 0;
            int best_q = 0;
            if (keep_first && k == 0 && s == 0) first_values.assign(1, best);
            const int top = lot_cap > 0 && k + 1 < remaining ? std::min(avail[k] - s, lot_cap) : avail[k] - s;
            for (int q = 1; q <= top; ++q) {
                gained += market_price(product, inventory + q - 1 + (interleave ? std::min(q - 1, r) : 0));
                const double v = gained * (1 + bonus[k]) + later(s + q) - rival_cost(q);
                if (keep_first && k == 0 && s == 0) first_values.push_back(v);
                if (v > best || (tie_now && v >= best - 1e-6)) best = v, best_q = q;
            }
            cur[s] = best;
            choice[k * width + s] = best_q;
        }
        std::swap(cur, next);
    }
    value = next[0];
    int sold = 0;
    for (int k = 0; k < remaining; ++k) {
        const int q = choice[k * width + sold];
        if (!k) first = q;
        revenue[k] = sale_value(product, inv0 + sold - cum_demand[k], q);
        sold += q;
    }
    return total - sold;
}

void demand_by_step(const agent::AgentObservation& obs, int item, int steps, int out[]) {
    for (int k = 0; k < steps; ++k) {
        const int step = obs.step + k;
        int d = 0;
        if (step % 4 == 0)
            for (int s = 0; s < obs.n_shops; ++s)
                if (SHOP_MASK[obs.shops[s]] & (1u << item)) d += SHOP_MULT[obs.shops[s]];
        if (step % HOURS == 0 && item != FERTILIZER) d += 1;
        out[k] = d;
    }
}

namespace {
// Net market inventory removed before each step: demand minus expected opponent sales.
void fill_flow(const agent::AgentObservation& obs, const double rival[HOURS][N_PRODUCTS], int p, int remaining,
               ProductSale& sale) {
    int demand[2 * HOURS]{};
    demand_by_step(obs, p, remaining + HOURS / 2, demand);
    double net = 0;
    for (int k = 0; k < remaining; ++k) sale.rival_units[k] = int(std::lround(std::max(0.0, rival[(obs.step + k) % HOURS][p])));
    for (int k = 0; k < remaining + HOURS / 2; ++k) {
        const double opponent = rival[(obs.step + k) % HOURS][p];
        net += demand[k] - (p == WHEAT || p == FERTILIZER ? opponent : std::max(0.0, opponent));
        sale.cum_demand[k + 1] = int(std::lround(net));
    }
    for (int k = 0; k < remaining + HOURS / 2 && k < 2 * HOURS; ++k) sale.step_demand[k] = demand[k];
    for (int k = 0; k < HOURS / 2; ++k) sale.future_rival[k] = int(std::lround(std::max(0.0, rival[(obs.step + remaining + k) % HOURS][p])));
}
}

DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options) {
    DayMarket m;
    const double* oracle = options.oracle;
    if (oracle) std::copy_n(oracle + dawn.day * HOURS * N_PRODUCTS, HOURS * N_PRODUCTS, &m.rival[0][0]);
    else forecast(dawn, history, m.rival, options.first_full, options.blend);
    if (!oracle && options.learned) options.learned(dawn, history, m.rival);
    m.hold_discount = options.hold_discount;
    m.rival_weight = options.rival_weight;
    m.grid_step = options.grid_step;
    m.interleave = options.interleave;
    m.tie_all = options.tie_all;
    m.wait_cost = options.wait_cost;
    m.wait_route_only = options.wait_route_only;
    m.lot_cap = options.lot_cap;
    m.floor_sales = options.floor_sales && dawn.day >= 6 && dawn.day < LAST_DAY;
    m.future_floor = options.future_floor;
    m.scenarios = options.scenarios;
    m.dawn_hours = dawn.day >= 1 && dawn.day < LAST_DAY ? options.dawn_sell : 0;
    m.dawn_start = options.dawn_start;
    int visible[N_PRODUCTS];
    visible_supply(dawn, visible);
    if (dawn.day == LAST_DAY - 1) std::copy_n(visible, N_PRODUCTS, m.hold_supply);
    if (dawn.day == LAST_DAY && !oracle)  // the opponent must liquidate too: its visible output comes early
        for (int p = 0; p < N_PRODUCTS; ++p)
            for (int h = 1; h <= 12; ++h) m.rival[h][p] += visible[p] / 12.0;
    return m;
}

double return_value(const agent::AgentObservation& dawn, const DayMarket& market, int p, int carried, int n, int hour, int hours,
                    int* sold_today) {
    ProductSale sale;
    sale.tie_now = p == MELON || market.tie_all;
    sale.product = p;
    sale.remaining = hours;
    sale.inv0 = dawn.market.inventory[p];
    sale.hold_discount = market.hold_discount;
    sale.hold_supply = market.hold_supply[p];
    sale.terminal = dawn.day == LAST_DAY;
    sale.rival_weight = sale.terminal ? 0 : market.rival_weight;
    sale.interleave = market.interleave;
    sale.wait_cost = market.wait_cost;
    sale.lot_cap = p != WHEAT && p != FERTILIZER ? market.lot_cap : 0;
    fill_flow(dawn, market.rival, p, hours, sale);
    for (int k = 0; k < hours; ++k) sale.avail[k] = dawn.own.shed[p] + (k >= hour ? n : 0);
    sale.total = dawn.own.shed[p] + carried;
    int first = 0;
    const int left = sale.solve(0, first);
    if (sold_today) *sold_today = sale.total - left;
    return sale.value;
}

void deposit_values(const agent::AgentObservation& dawn, const DayMarket& market, const int output[N_PRODUCTS], int hours,
                    double gain[N_PRODUCTS][HOURS]) {
    for (int p = 0; p < N_PRODUCTS; ++p) std::fill_n(gain[p], HOURS, 0.0);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        const int c = output[p];
        if (c <= 0) continue;
        const double none = return_value(dawn, market, p, c, 0, HOURS, hours);
        // Hours 0, 2, ..., 22 and the last market; linear in between.
        double at[HOURS];
        int grid[HOURS], m = 0;
        for (int h = 0; h < hours; h += market.grid_step) grid[m++] = h;
        if (grid[m - 1] != hours - 1) grid[m++] = hours - 1;
        for (int k = 0; k < m; ++k) at[k] = (return_value(dawn, market, p, c, c, grid[k], hours) - none) / c;
        for (int k = 0; k + 1 < m; ++k)
            for (int h = grid[k]; h <= grid[k + 1]; ++h)
                gain[p][h] = at[k] + (at[k + 1] - at[k]) * (h - grid[k]) / double(grid[k + 1] - grid[k]);
        // Later deposits never earn more than earlier ones (a unit can wait in the shed).
        for (int h = 1; h < hours; ++h) gain[p][h] = std::min(gain[p][h], gain[p][h - 1]);
        for (int h = 0; h < hours; ++h) gain[p][h] = std::max(0.0, gain[p][h]);
    }
}


void choose_sales(const agent::AgentObservation& obs, const int incoming[HOURS][N_PRODUCTS], int hours, const DayMarket& market,
                  const int reserve[N_PRODUCTS], int night_room, int sell[N_PRODUCTS], double* charge_out, const double* cash_need) {
    std::fill_n(sell, N_PRODUCTS, 0);
    const int h = obs.hour;
    const int remaining = hours - h;
    if (remaining <= 0) return;
    if (market.dawn_hours > 0 && h >= market.dawn_start && h < market.dawn_start + market.dawn_hours) {  // test-opponent window
        for (int p = 0; p < N_PRODUCTS; ++p) sell[p] = std::max(0, obs.own.shed[p] - reserve[p]);
        return;
    }
    std::vector<ProductSale> sales;
    for (int p = 0; p < N_PRODUCTS; ++p) {
        ProductSale sale;
        sale.tie_now = p == MELON || market.tie_all;
        sale.product = p;
        sale.remaining = remaining;
        sale.inv0 = obs.market.inventory[p];
        sale.hold_discount = market.hold_discount;
        sale.hold_supply = market.hold_supply[p];
        sale.terminal = obs.day == LAST_DAY;
        const int now_received = h > 0 ? incoming[h - 1][p] : 0;
        for (int k = 0; k < remaining; ++k)
            sale.avail[k] = std::max(0, obs.own.shed[p] + (incoming[h + k][p] - now_received) - reserve[p]);
        sale.total = sale.avail[remaining - 1];
        if (sale.total <= 0) continue;
        sale.rival_weight = sale.terminal ? 0 : market.rival_weight;
        sale.interleave = market.interleave;
        sale.wait_cost = market.wait_route_only ? 0 : market.wait_cost;
        if (market.carry[p] && market.carry_hold > 0) sale.hold_discount = market.carry_hold;  // regime M: overnight hold
        if (market.dawn[p] && market.carry_wait > 0) sale.wait_cost = market.carry_wait;       // regime M: dawn-first
        sale.lot_cap = p != WHEAT && p != FERTILIZER ? market.lot_cap : 0;
        sale.floor = market.floor_sales && p >= CARROT && p <= WOOL;
        sale.future_floor = market.future_floor;
        fill_flow(obs, market.rival, p, remaining, sale);
        sales.push_back(sale);
    }
    auto run = [&](double charge) {
        int left = 0;
        for (auto& sale : sales) left += sale.solve(charge, sell[sale.product]);
        return left;
    };
    if (market.dawn_hours > 0)  // test opponents keep stock for tomorrow's window unless it does not fit tonight
        for (auto& sale : sales) sale.hold_discount = 10;
    // Night room: a per-unit charge on stock left after the last market, as small as fits.
    double charge = 0;
    if (run(0) > night_room && night_room >= 0 && obs.day != LAST_DAY) {
        double low = 0, high = 1000;
        if (run(high) <= night_room)
            for (int iteration = 0; iteration < 12; ++iteration) {
                const double mid = 0.5 * (low + high);
                (run(mid) <= night_room ? high : low) = mid;
            }
        charge = high;
        run(charge);
    }
    if (charge_out) *charge_out = charge;  // > 0: tonight's shed room binds
    // cashsell: while the schedule leaves a planned purchase unfunded, a revenue bonus on the sales up to that step (bisection).
    for (int round = 0; cash_need && round < 3; ++round) {
        auto covered = [&](int until) {
            double cum = 0;
            for (int k = 0; k <= until; ++k)
                for (const auto& sale : sales) cum += sale.revenue[k];
            return cum >= cash_need[until];
        };
        int deadline = -1;
        for (int k = 0; k < remaining && deadline < 0; ++k)
            if (!covered(k)) deadline = k;
        if (deadline < 0) break;
        double base[N_PRODUCTS][HOURS];
        for (size_t i = 0; i < sales.size(); ++i) std::copy_n(sales[i].bonus, HOURS, base[i]);
        auto with = [&](double lambda) {
            for (size_t i = 0; i < sales.size(); ++i)
                for (int k = 0; k <= deadline; ++k) sales[i].bonus[k] = base[i][k] + lambda;
            run(charge);
            return covered(deadline);
        };
        double low = 0, high = 4;
        if (with(high))
            for (int iteration = 0; iteration < 10; ++iteration) {
                const double mid = 0.5 * (low + high);
                (with(mid) ? high : low) = mid;
            }
        const bool funded = with(high);
        if (!funded) break;  // cannot be covered by then: the most it can raise
    }
    // Scenario seller: this hour's units by their mean value over sampled opponent paths (night room free).
    if (market.scenarios > 0 && charge == 0)
        for (auto& sale : sales) {
            const int p = sale.product;
            double any = 0;
            for (int k = 0; k < HOURS; ++k) any += std::max(0.0, market.rival[k][p]);
            if (any <= 0) continue;
            std::vector<double> mean;
            std::mt19937 rng(uint32_t(obs.step * 7919 + p * 104729 + obs.day));
            for (int k = 0; k < market.scenarios; ++k) {
                double path[HOURS][N_PRODUCTS];
                std::copy_n(&market.rival[0][0], HOURS * N_PRODUCTS, &path[0][0]);
                for (int t = 0; t < HOURS; ++t)
                    if (path[t][p] > 0) path[t][p] = std::poisson_distribution<int>(path[t][p])(rng);
                ProductSale one = sale;
                fill_flow(obs, path, p, remaining, one);
                one.keep_first = true;
                int first = 0;
                one.solve(0, first);
                if (mean.empty()) mean.assign(one.first_values.size(), 0.0);
                for (size_t q = 0; q < mean.size(); ++q) mean[q] += one.first_values[q];
            }
            int best_q = 0;
            for (size_t q = 1; q < mean.size(); ++q)
                if (mean[q] > mean[best_q] || (sale.tie_now && mean[q] >= mean[best_q] - 1e-6)) best_q = int(q);
            sell[p] = best_q;
        }
}
}

namespace dc11 {
void timing_scenario(const double rival[HOURS][N_PRODUCTS], int from, int hours, int block, double out[HOURS][N_PRODUCTS],
                     int shift, int length) {
    std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &out[0][0]);
    const int start = from + shift + (block == 0 ? 1 : block == 1 ? 7 : 14);
    for (int p = 0; p < N_PRODUCTS; ++p) {
        double total = 0;
        for (int h = from; h < hours; ++h) total += std::max(0.0, rival[h][p]);
        for (int h = from; h < hours; ++h)
            if (out[h][p] > 0) out[h][p] = 0;
        for (int h = start; h < start + length && h < hours; ++h) out[h][p] += total / length;
    }
}
}
