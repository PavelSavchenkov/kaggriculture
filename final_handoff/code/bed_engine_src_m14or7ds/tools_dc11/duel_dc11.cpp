// Same-world duels (local, sep28_top_lb_imitation): two live dc11 agents replay a recorded Kaggle game's world (its seed and
// config). The arm (BC_OPUS_MODEL) takes one seat, the other seat's model comes from the list line. Our subs reproduce their
// Kaggle play to the dollar, so with the sub in its own seat the game equals the real one until the arm's play differs from the
// recorded player's. REPLAY_SHOPS=1 keeps the shops as recorded (they follow the weed RNG and drift once play differs).
// DUEL_TRACES=<dir>: writes <dir>/<episode>.txt. Per dawn money of both seats in the csv (d0..d24) against the recording's.
// DUEL_DAYS=<file>: per day, hour band (0-2, 3-11, 12-20, 21-23) and product units sold and revenue of arm / opp (live) and real_arm / real_opp (the recording), plus
// spend and hires (product "spend" / "hires"); revenue split over a step's products by units x mean market price, scaled to the
// step's actual revenue.
// DUEL_HERD=<file>: per dawn geese / cows / sheep / field plants of arm, opp, real_arm, real_opp.
// DUEL_SALES="p p ..." (product ids, e.g. "3 6 7" strawberries / milk / wool) with DUEL_SALES_DAYS="first last": sale transplant -
// on those days the arm's sell orders for those products are replaced by the recorded seat's units sold that hour (capped by the
// arm's shed stock). Units the shed could not cover carry to later hours (backlog); in the last hour the arm's own sale stands if
// larger (night room). Tells whether the recorded player's selling alone reproduces its result.
// DUEL_SELL=<file>: per hour, thin product (strawberry, milk, wool) and seat: shed stock at the turn, units sold, market inventory, for
// arm / opp (live) and real_arm / real_opp (the recording).
// usage: duel_dc11 list.txt threads out.csv   (list line: trace arm_seat opponent_model.bin)
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

using namespace dc10;

namespace {
std::string dc11_sidecar(const std::string& model) {
    char text[4096] = {0};  // options lines can exceed 255 chars
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        if (!std::fgets(text, sizeof text, file)) text[0] = 0;
        std::fclose(file);
    }
    std::string s = text;
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}

struct Game {
    std::string trace, opponent;
    int seat = 0;  // the arm's seat
};

struct Result {
    uint64_t seed = 0;
    double money[2] = {0, 0}, real[2] = {0, 0};
    std::vector<double> dawn[2], real_dawn[2];
    std::vector<std::array<int, 4>> herd[2], real_herd[2];
    std::vector<std::array<int, 7>> sells;  // DUEL_SELL rows: who, day, hour, product, shed, sold, inventory  // per dawn: geese, cows, sheep, field plants
    // [who: seat 0, seat 1 live, seat 0, seat 1 recorded][day][product, spend, hires]
    std::vector<double> units, revenue;
};
constexpr int N_COLS = N_PRODUCTS + 2;
constexpr const char* NAMES[N_COLS] = {"wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer", "spend", "hires"};
constexpr int N_BANDS = 4;  // hours 0-2, 3-11, 12-20, 21-23
inline int band_of(int hour) { return hour <= 2 ? 0 : hour <= 11 ? 1 : hour <= 20 ? 2 : 3; }
inline size_t cell(int who, int day, int band, int item) { return ((size_t(who) * 30 + day) * N_BANDS + band) * N_COLS + item; }

// One step's sales / spend of both seats (before -> after) into who_base + seat.
void account(const Sim& before, const Sim& after, int who_base, Result& r) {
    const int day = std::min(29, before.st.step / HOURS), band = band_of(before.st.step % HOURS);
    double price[N_PRODUCTS] = {}, estimate[2] = {0, 0};
    int du[2][N_PRODUCTS];
    for (int i = 0; i < N_PRODUCTS; ++i) {
        for (int k = 0; k < 2; ++k) du[k][i] = after.st.farms[k].sold_units[i] - before.st.farms[k].sold_units[i];
        if (const int n = du[0][i] + du[1][i]; n > 0) {
            for (int j = 0; j < n; ++j) price[i] += market_price(i, before.st.market.inventory[i] + j);
            price[i] /= n;
            for (int k = 0; k < 2; ++k) estimate[k] += du[k][i] * price[i];
        }
    }
    for (int k = 0; k < 2; ++k) {
        const Farm &f0 = before.st.farms[k], &f1 = after.st.farms[k];
        const double actual = f1.sell_revenue - f0.sell_revenue;
        for (int i = 0; i < N_PRODUCTS; ++i)
            if (du[k][i] > 0) {
                r.units[cell(who_base + k, day, band, i)] += du[k][i];
                r.revenue[cell(who_base + k, day, band, i)] += estimate[k] > 0 ? du[k][i] * price[i] * actual / estimate[k] : 0;
            }
        r.revenue[cell(who_base + k, day, band, N_PRODUCTS)] += f1.total_spend - f0.total_spend;
        r.units[cell(who_base + k, day, band, N_PRODUCTS + 1)] += std::max(0, f1.hires_today - f0.hires_today);
    }
}

Result play(const Game& game) {
    const Replay replay = load_replay(game.trace);
    const auto original = replay_states(replay);
    Sim sim(replay.config);
    kag::agents::bc_overhaul::Agent agents[2];
    const std::string models[2] = {game.seat == 0 ? std::getenv("BC_OPUS_MODEL") : game.opponent,
                                   game.seat == 1 ? std::getenv("BC_OPUS_MODEL") : game.opponent};
    for (int s = 0; s < 2; ++s) {
        agents[s].model_path = models[s];
        const std::string sidecar = dc11_sidecar(models[s]);
        agents[s].options_text = !sidecar.empty() ? sidecar : "-";
        agents[s].reset(kag::agent::runtime::make_agent_init(sim, s));
    }
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    const bool pin_shops = std::getenv("REPLAY_SHOPS") != nullptr;
    bool transplant[N_PRODUCTS]{};
    int sales_first = 0, sales_last = 29;
    int backlog[N_PRODUCTS]{};
    if (const char* t = std::getenv("DUEL_SALES")) {
        std::istringstream in(t);
        for (int p; in >> p;) transplant[p] = true;
        if (const char* d = std::getenv("DUEL_SALES_DAYS")) std::istringstream(d) >> sales_first >> sales_last;
    }
    std::vector<std::array<Action, 2>> turns;
    Result r;
    const bool days = std::getenv("DUEL_DAYS") != nullptr;
    const bool sell_log = std::getenv("DUEL_SELL") != nullptr;
    r.units.assign(4 * 30 * N_BANDS * N_COLS, 0), r.revenue.assign(4 * 30 * N_BANDS * N_COLS, 0);
    for (int step = 0; !sim.st.done; ++step) {
        if (step % HOURS == 0)
            for (int s = 0; s < 2; ++s) {
                r.dawn[s].push_back(sim.st.farms[s].money);
                r.real_dawn[s].push_back(step < int(original.size()) ? original[step].st.farms[s].money : -1);
                auto count = [](const Farm& f) {
                    std::array<int, 4> n{};
                    for (int y = 0; y < BOARD; ++y)
                        for (int x = 0; x < BOARD; ++x) {
                            const Tile& t = f.tiles[y][x];
                            if (t.has_animal) ++n[t.what - GOOSE];
                            n[3] += t.kind == T_PLANT;
                        }
                    return n;
                };
                r.herd[s].push_back(count(sim.st.farms[s]));
                r.real_herd[s].push_back(step < int(original.size()) ? count(original[step].st.farms[s]) : std::array<int, 4>{-1, -1, -1, -1});
            }
        Action a[2];
        for (int s = 0; s < 2; ++s) agents[s].act(kag::agent::runtime::make_observation(sim, s), budget, a[s]);
        if (const int day = step / HOURS; day >= sales_first && day <= sales_last && step + 1 < int(original.size())) {
            const int s = game.seat;
            Action& x = a[s];
            int k = 0;
            int own[N_PRODUCTS]{};
            for (int j = 0; j < x.n_orders; ++j)
                if (x.orders[j].op == M_SELL && x.orders[j].item < N_PRODUCTS && transplant[x.orders[j].item]) own[x.orders[j].item] += x.orders[j].n;
                else x.orders[k++] = x.orders[j];
            x.n_orders = k;
            const bool last = step % HOURS == HOURS - 1;
            for (int p = 0; p < N_PRODUCTS; ++p) {
                if (!transplant[p]) continue;
                backlog[p] += original[step + 1].st.farms[s].sold_units[p] - original[step].st.farms[s].sold_units[p];
                const int n = std::min<int>(std::max(backlog[p], last ? own[p] : 0), sim.st.farms[s].shed[p]);
                backlog[p] = std::max(0, backlog[p] - n);
                if (n > 0 && x.n_orders < 10) x.orders[x.n_orders++] = {M_SELL, uint8_t(p), n};
            }
        }
        turns.push_back({a[0], a[1]});
        const Sim before = days || sell_log ? sim : Sim(replay.config);
        sim.step(a[0], a[1]);
        if (sell_log)
            for (int prod : {int(STRAWBERRY), int(MILK), int(WOOL)}) {
                for (int s = 0; s < 2; ++s)
                    r.sells.push_back({s, step / HOURS, step % HOURS, prod, int(before.st.farms[s].shed[prod]),
                                       sim.st.farms[s].sold_units[prod] - before.st.farms[s].sold_units[prod], before.st.market.inventory[prod]});
                if (step + 1 < int(original.size()))
                    for (int s = 0; s < 2; ++s)
                        r.sells.push_back({2 + s, step / HOURS, step % HOURS, prod, int(original[step].st.farms[s].shed[prod]),
                                           original[step + 1].st.farms[s].sold_units[prod] - original[step].st.farms[s].sold_units[prod],
                                           original[step].st.market.inventory[prod]});
            }
        if (days) {
            account(before, sim, 0, r);
            if (step + 1 < int(original.size())) account(original[step], original[step + 1], 2, r);
        }
        if (pin_shops && step + 1 < int(original.size()))
            for (int k = 0; k < sim.st.n_shops && k < original[step + 1].st.n_shops; ++k) sim.st.shops[k] = original[step + 1].st.shops[k];
    }
    if (const char* dir = std::getenv("DUEL_TRACES")) {
        const std::string name = game.trace.substr(game.trace.find_last_of('/') + 1);
        save_replay(std::string(dir) + "/" + name, replay.config, turns);
    }
    r.seed = replay.config.seed;
    for (int s = 0; s < 2; ++s) r.money[s] = sim.st.farms[s].money, r.real[s] = original.back().st.farms[s].money;
    return r;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: duel_dc11 list.txt threads out.csv\n";
        return 2;
    }
    setenv("DC10_NO_DEADLINE", "1", 0);  // deterministic games (as full_games_dc11)
    std::vector<Game> games;
    std::ifstream list(argv[1]);
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        Game g;
        if (fields >> g.trace >> g.seat >> g.opponent) games.push_back(g);
    }
    const int threads = std::atoi(argv[2]);
    std::vector<Result> results(games.size());
    std::atomic<size_t> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (size_t job; (job = next++) < games.size();) results[job] = play(games[job]);
        });
    for (auto& t : pool) t.join();
    std::ofstream out(argv[3]);
    out << "trace,arm_seat,seed,arm,opp,margin,real_arm,real_opp,real_margin";
    for (int d = 0; d < 25; ++d) out << ",arm_d" << d << ",opp_d" << d << ",real_arm_d" << d << ",real_opp_d" << d;
    out << "\n";
    for (size_t i = 0; i < games.size(); ++i) {
        const auto& r = results[i];
        const int a = games[i].seat, o = 1 - a;
        out << games[i].trace << "," << a << "," << r.seed << "," << r.money[a] << "," << r.money[o] << "," << r.money[a] - r.money[o] << ","
            << r.real[a] << "," << r.real[o] << "," << r.real[a] - r.real[o];
        for (int d = 0; d < 25; ++d) {
            auto at = [&](const std::vector<double>& v) { return d < int(v.size()) ? v[d] : -1; };
            out << "," << at(r.dawn[a]) << "," << at(r.dawn[o]) << "," << at(r.real_dawn[a]) << "," << at(r.real_dawn[o]);
        }
        out << "\n";
    }
    if (const char* path = std::getenv("DUEL_SELL")) {
        std::ofstream out(path);
        out << "trace,who,day,hour,product,shed,sold,inv\n";
        for (size_t g = 0; g < games.size(); ++g) {
            const int a = games[g].seat;
            for (const auto& x : results[g].sells) {
                const bool live = x[0] < 2, arm = (x[0] % 2) == a;
                out << games[g].trace << "," << (live ? (arm ? "arm" : "opp") : (arm ? "real_arm" : "real_opp")) << "," << x[1] << "," << x[2] << ","
                    << x[3] << "," << x[4] << "," << x[5] << "," << x[6] << "\n";
            }
        }
    }
    if (const char* path = std::getenv("DUEL_HERD")) {  // per dawn geese / cows / sheep / plants of arm, opp, real_arm, real_opp
        std::ofstream herd(path);
        herd << "trace,who,day,geese,cows,sheep,plants\n";
        for (size_t g = 0; g < games.size(); ++g) {
            const int a = games[g].seat, o = 1 - a;
            const std::pair<const char*, const std::vector<std::array<int, 4>>*> rows[4] = {
                {"arm", &results[g].herd[a]}, {"opp", &results[g].herd[o]}, {"real_arm", &results[g].real_herd[a]}, {"real_opp", &results[g].real_herd[o]}};
            for (const auto& [name, v] : rows)
                for (size_t d = 0; d < v->size(); ++d)
                    herd << games[g].trace << "," << name << "," << d << "," << (*v)[d][0] << "," << (*v)[d][1] << "," << (*v)[d][2] << "," << (*v)[d][3] << "\n";
        }
    }
    if (const char* path = std::getenv("DUEL_DAYS")) {
        std::ofstream days(path);
        days << "trace,who,day,band,item,units,revenue\n";
        for (size_t g = 0; g < games.size(); ++g) {
            const int a = games[g].seat;
            const char* names[4] = {a == 0 ? "arm" : "opp", a == 0 ? "opp" : "arm", a == 0 ? "real_arm" : "real_opp", a == 0 ? "real_opp" : "real_arm"};
            for (int w = 0; w < 4; ++w)
                for (int d = 0; d < 30; ++d)
                    for (int b = 0; b < N_BANDS; ++b)
                    for (int i = 0; i < N_COLS; ++i) {
                        const double u = results[g].units[cell(w, d, b, i)], v = results[g].revenue[cell(w, d, b, i)];
                        if (u || v)
                            days << games[g].trace << "," << names[w] << "," << d << "," << b << "," << NAMES[i]
                                 << "," << u << "," << v << "\n";
                    }
        }
    }
    return 0;
}
