// Opponent sale forecast audit: for each trace (list line: trace our_seat), replays the game with our
// seat's History and at each dawn of days 3-28 forecasts the opponent's units sold that day per
// product (not wheat or fertilizer) three ways: trailing (3-day mean of inferred sales), blend
// (melon: visible ripe/held output; others 0.5 trailing + 0.5 visible) and adapt (3-day sell-through
// rate x (inferred stock + visible)). Prints the mean absolute error per product and method against
// the opponent's actual sales in the trace.
// FORECAST_ROWS=<csv>: also write one row per trace, day and product (trailing, visible, stock,
// sell-through, day, actual) for fitting a forecast model.
// usage: forecast_audit list.txt
#include "source/history.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

using namespace dc10;

namespace {
constexpr double FIT[N_PRODUCTS][6] = {
    {0, 0, 0, 0, 0, 0}, {-0.0244, -0.0077, 0.9766, 0.0409, 0.0228, -0.0084}, {0.0194, 0.0041, 0.9987, -0.0005, -0.0385, 0.0131},
    {0.7552, 0.2545, 0.7149, -0.2062, 1.3505, -0.3041}, {-0.0006, 0.7359, 0.6489, 0.4380, 0.0008, -0.0040},
    {-0.0533, -0.0282, 0.9668, 0.0857, 0.1577, -0.0302}, {0.2635, 0.2251, 0.5319, -0.1765, 6.3437, -0.7765},
    {-0.0125, 0.3442, 0.5496, -0.0131, 4.9130, -0.6515}, {0, 0, 0, 0, 0, 0}};  // compiler.cpp FORECAST_FIT
void visible(const agent::AgentObservation& o, int out[N_PRODUCTS]) {
    std::fill_n(out, N_PRODUCTS, 0);
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) {
            const Tile& t = o.opponent().tiles[y][x];
            if (t.has_animal) out[ANIMALS[t.what - GOOSE].product] += t.yield_units;
            else if (t.kind == T_PLANT && o.day - t.planted_day >= CROPS[t.what].first_yield_day) out[t.what] += t.yield_units;
        }
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: forecast_audit list.txt\n";
        return 2;
    }
    std::ifstream list(argv[1]);
    std::ofstream rows;
    if (const char* path = std::getenv("FORECAST_ROWS")) {
        rows.open(path);
        rows << "trace,day,product,trailing,visible,stock,through,actual\n";
    }
    double err[7][N_PRODUCTS]{}, actual_sum[N_PRODUCTS]{};
    double shape_err[N_PRODUCTS]{}, profile_err[N_PRODUCTS]{};  // timing: half the L1 distance of 6-hour bucket shares, predicted vs actual
    long shape_n[N_PRODUCTS]{};
    long n[N_PRODUCTS]{};
    for (std::string line; std::getline(list, line);) {
        std::istringstream in(line);
        std::string trace;
        int seat = 0;
        if (!(in >> trace >> seat)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        History history;
        double forecast[7][N_PRODUCTS]{}, features[4][N_PRODUCTS]{};
        int32_t sold_at_dawn[N_PRODUCTS]{}, sold_before[N_PRODUCTS]{};
        double predicted_share[N_PRODUCTS][4]{}, profile_share[N_PRODUCTS][4]{}, actual_bucket[N_PRODUCTS][4]{};
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const auto obs = agent::runtime::make_observation(sim, seat);
            if (obs.hour == 0) {
                const int day = obs.day;
                if (day >= 4 && day <= 29)  // score yesterday's forecast
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        if (p == WHEAT || p == FERTILIZER) continue;
                        const double actual = sim.st.farms[1 - seat].sold_units[p] - sold_at_dawn[p];
                        for (int m = 0; m < 7; ++m) err[m][p] += std::abs(forecast[m][p] - actual);
                        if (rows.is_open())
                            rows << trace << ',' << day - 1 << ',' << p << ',' << features[0][p] << ',' << features[1][p] << ','
                                 << features[2][p] << ',' << features[3][p] << ',' << actual << '\n';
                        actual_sum[p] += actual;
                        ++n[p];
                    }
                if (day >= 4 && day <= 29)
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        double total = 0;
                        for (int b = 0; b < 4; ++b) total += actual_bucket[p][b];
                        double pred = 0;
                        for (int b = 0; b < 4; ++b) pred += predicted_share[p][b];
                        if (total <= 0 || pred <= 0 || p == WHEAT || p == FERTILIZER) continue;
                        double l1 = 0;
                        for (int b = 0; b < 4; ++b) l1 += std::abs(predicted_share[p][b] / pred - actual_bucket[p][b] / total);
                        shape_err[p] += 0.5 * l1;
                        double prof = 0, l1p = 0;
                        for (int b = 0; b < 4; ++b) prof += profile_share[p][b];
                        for (int b = 0; b < 4; ++b)
                            l1p += std::abs((prof > 0 ? profile_share[p][b] / prof : predicted_share[p][b] / pred) - actual_bucket[p][b] / total);
                        profile_err[p] += 0.5 * l1p;
                        ++shape_n[p];
                    }
                for (int p = 0; p < N_PRODUCTS; ++p) {
                    sold_at_dawn[p] = sim.st.farms[1 - seat].sold_units[p];
                    for (int b = 0; b < 4; ++b) actual_bucket[p][b] = 0;
                }
                double rival[HOURS][N_PRODUCTS];
                history.expected(day, 3, rival);
                int vis[N_PRODUCTS];
                visible(obs, vis);
                for (int p = 0; p < N_PRODUCTS; ++p) {
                    double trailing = 0;
                    for (int h = 0; h < HOURS; ++h) trailing += std::max(0.0, rival[h][p]);
                    double profile[HOURS];
                    history.hour_profile(day, p, profile);
                    for (int b = 0; b < 4; ++b) {
                        predicted_share[p][b] = profile_share[p][b] = 0;
                        for (int h = 6 * b; h < 6 * b + 6; ++h)
                            predicted_share[p][b] += std::max(0.0, rival[h][p]), profile_share[p][b] += profile[h];
                    }
                    forecast[0][p] = trailing;
                    forecast[1][p] = p == MELON ? vis[p] : 0.5 * trailing + 0.5 * vis[p];
                    forecast[2][p] = history.sell_through(day, p, 3, 0.6) * (vis[p] + history.opponent_stock()[p]);
                    forecast[3][p] = p == MELON ? vis[p] : 0.3 * trailing + 0.7 * vis[p];
                    const int stocked = vis[p] + history.opponent_stock()[p];
                    forecast[4][p] = p == MELON ? stocked : 0.5 * trailing + 0.5 * stocked;
                    features[0][p] = trailing, features[1][p] = vis[p], features[2][p] = history.opponent_stock()[p];
                    features[3][p] = history.sell_through(day, p, 3, 0.6);
                    const auto& c = FIT[p];
                    auto fit = [&](double t, double v, double st, double th, int d) {
                        return std::max(0.0, c[0] * t + c[1] * v + c[2] * st + c[3] * th * (v + st) + c[4] * d / 30.0 + c[5]);
                    };
                    forecast[5][p] = fit(trailing, vis[p], history.opponent_stock()[p], features[3][p], day);
                    double eb = 0, ef = 0;
                    for (int k = std::max(1, day - 3); k < day; ++k) {
                        double past[HOURS][N_PRODUCTS];
                        history.expected(k, 3, past);
                        double t = 0;
                        for (int h = 0; h < HOURS; ++h) t += std::max(0.0, past[h][p]);
                        const double v = history.dawn_visible(k, p), st = history.dawn_stock(k, p);
                        eb += std::abs((p == MELON ? v : 0.5 * t + 0.5 * v) - history.sold_on(k, p));
                        ef += std::abs(fit(t, v, st, history.sell_through(k, p, 3, 0.6), k) - history.sold_on(k, p));
                    }
                    forecast[6][p] = ef < eb ? forecast[5][p] : forecast[1][p];
                }
            }
            history.observe(obs, replay.turns[s][seat]);
            for (int p = 0; p < N_PRODUCTS; ++p) sold_before[p] = sim.st.farms[1 - seat].sold_units[p];
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            for (int p = 0; p < N_PRODUCTS; ++p) actual_bucket[p][obs.hour / 6] += sim.st.farms[1 - seat].sold_units[p] - sold_before[p];
        }
    }
    const char* names[7] = {"trailing", "blend", "adapt", "blend70", "bl+stock", "fitted", "select"};
    std::printf("%-11s %8s", "product", "mean/day");
    for (auto name : names) std::printf(" %9s", name);
    std::printf("\n");
    for (int p = 0; p < N_PRODUCTS; ++p) {
        if (!n[p]) continue;
        std::printf("%-11d %8.2f", p, actual_sum[p] / n[p]);
        for (int m = 0; m < 7; ++m) std::printf(" %9.2f", err[m][p] / n[p]);
        std::printf("   timing error %.2f profile %.2f", shape_n[p] ? shape_err[p] / shape_n[p] : 0.0,
                    shape_n[p] ? profile_err[p] / shape_n[p] : 0.0);
        std::printf("\n");
    }
    return 0;
}
