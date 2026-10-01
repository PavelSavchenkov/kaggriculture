// Full games against frozen replays: bc_opus plays one seat of a recorded episode (same
// seed and config) while the other seat repeats its recorded actions. The replayed player
// does not react to us; orders that depended on the original market may fail.
// usage: replay_games list.txt threads out.csv   (list line: trace replayed_seat)
#include "agent/bc_opus/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <thread>
#include "tools/shop_crn.hpp"

using namespace dc10;

#ifdef MINE_DC11  // replay_games_dc11: our seat plays the vendored dc11 agent (model from BC_OPUS_MODEL; options from DC11_OPTIONS, else <model>.dc11)
#include "agent/bc_overhaul/source/agent.hpp"
namespace {
// <model>.dc11: the model's dc11 options (as the Local-LB bridge reads them), "" if none.
inline std::string dc11_sidecar(const std::string& model) {
    std::string s;
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {  // step 67: the whole first line (no 255-character limit)
            s += chunk;
            if (s.back() == '\n') break;
        }
        std::fclose(file);
    }
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}
struct Mine : kag::agents::bc_overhaul::Agent {
    Mine() {
        if (const char* m = std::getenv("BC_OPUS_MODEL")) model_path = m;
        if (!std::getenv("DC11_OPTIONS")) options_text = dc11_sidecar(model_path);  // "" -> dc11 defaults
    }
};
}  // namespace
#else
using Mine = kag::agents::bc_opus::Agent;
#endif

namespace {
struct Game {
    std::string trace;
    int replayed = 0;
};

struct Result {
    uint64_t seed = 0;
    double own = 0, rival = 0;              // this game
    double original_rival = 0, original_other = 0;  // the recorded episode's final money
    int first_shop = -1;                  // kind of the first town shop (unlocks on day 3)
    int d4[3] = {-1, -1, -1}, d7[3] = {-1, -1, -1}, d10[3] = {-1, -1, -1};  // our geese, cows, sheep at dawns 4, 7, 10
    double cash[4] = {-1, -1, -1, -1};  // our money at dawns 3-6 (idle cash; DSM keeps ~$15-54)
    int opp_div_day = -1;  // first dawn where the replayed farm's tiles differ from the recording (its orders failed), -1: never
    // REPLAY_LEDGER: sales per [who: 0 us, 1 replayed][day block: 0-9, 10-19, 20+][product][hour]; revenue split over a step's
    // products by units x mean market price over the step's supply, scaled to the step's actual revenue.
    std::vector<float> units, revenue;
};
inline size_t ledger_index(int who, int day, int item, int hour) { return ((who * 3 + std::min(day / 10, 2)) * N_ITEMS + item) * HOURS + hour; }

Result play(const Game& game) {
    const Replay replay = load_replay(game.trace);
    const auto original = replay_states(replay);
    const int seat = 1 - game.replayed;
    Sim sim(replay.config);
#ifdef MINE_DC11
    std::vector<double> flow(30 * HOURS * N_PRODUCTS, 0.0);  // REPLAY_ORACLE: the replayed player's accepted sells - buys per hour
    if (std::getenv("REPLAY_ORACLE")) {
        for (size_t s = 0; s < replay.turns.size() && s < original.size(); ++s) {
            Sim at = original[s];
            const auto acc = at.sanitize_joint_actions(replay.turns[s][0], replay.turns[s][1])[game.replayed];
            double* f = &flow[s * N_PRODUCTS];
            for (int k = 0; k < acc.n_orders; ++k) {
                const auto& o = acc.orders[k];
                if (o.op == M_SELL && o.item < N_PRODUCTS) f[o.item] += o.n;
                if (o.op == M_BUY_PRODUCT && o.item < N_PRODUCTS) f[o.item] -= o.n;
            }
        }
        kag::agents::bc_overhaul::dc11_oracle_flow = flow.data();
    } else kag::agents::bc_overhaul::dc11_oracle_flow = nullptr;
#endif
    int crn_shops = 0;  // SHOP_CRN: shops fixed so far
    Mine mine;
    mine.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    const bool trace = std::getenv("BC_REPLAY_TRACE") != nullptr;  // per dawn: farms and the compile report
    // REPLAY_SHOPS: every shop as recorded (shops follow the weed RNG, so they drift once our play differs).
    // REPLAY_WEEDS: the replayed player's empty/weed tiles as recorded (our empty tiles shift its weed draws).
    // Ported from work/sep25_bc_weakness (port_replay_shops.py, port_replay_weeds.py). Evaluation only.
    const bool pin_shops = std::getenv("REPLAY_SHOPS") != nullptr, pin_weeds = std::getenv("REPLAY_WEEDS") != nullptr;
    Result r;
    const bool ledger = std::getenv("REPLAY_LEDGER") != nullptr;
    if (ledger) r.units.assign(2 * 3 * N_ITEMS * HOURS, 0), r.revenue.assign(2 * 3 * N_ITEMS * HOURS, 0);
    for (int step = 0; !sim.st.done; ++step) {
        if (step >= 3 * 24 && step <= 6 * 24 && step % 24 == 0) r.cash[step / 24 - 3] = sim.st.farms[seat].money;
        if (step == 4 * 24 || step == 7 * 24 || step == 10 * 24) {
            int* n = step == 4 * 24 ? r.d4 : step == 7 * 24 ? r.d7 : r.d10;
            n[0] = n[1] = n[2] = 0;
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x)
                    if (const Tile& t = sim.st.farms[seat].tiles[y][x]; t.has_animal) ++n[t.what - GOOSE];
        }
        if (step % 24 == 0 && r.opp_div_day < 0 && step < int(original.size())) {  // the frozen opponent's farm vs the recording
            const Farm &f = sim.st.farms[game.replayed], &g = original[step].st.farms[game.replayed];
            for (int y = 0; y < BOARD && r.opp_div_day < 0; ++y)
                for (int x = 0; x < BOARD; ++x)
                    if (f.tiles[y][x].kind != g.tiles[y][x].kind || f.tiles[y][x].what != g.tiles[y][x].what ||
                        f.tiles[y][x].has_animal != g.tiles[y][x].has_animal) {
                        r.opp_div_day = step / 24;
                        break;
                    }
        }
        Action a;
        mine.act(kag::agent::runtime::make_observation(sim, seat), budget, a);
        if (trace && step % 24 == 0) {
            auto count = [&](const Farm& f, bool animals) {
                int n = 0;
                for (int y = 0; y < 10; ++y)
                    for (int x = 0; x < 10; ++x) n += animals ? f.tiles[y][x].has_animal : f.tiles[y][x].kind == T_PLANT;
                return n;
            };
            const Farm &o = sim.st.farms[seat], &v = sim.st.farms[game.replayed];
            const auto& rep = mine.reports().back();
            std::printf("day %2d cash %7.0f/%7.0f animals %2d/%2d plants %2d/%2d | status %d fallback %d | %s\n", step / 24, o.money,
                        v.money, count(o, true), count(v, true), count(o, false), count(v, false), rep.status, rep.fallback,
                        rep.reason.substr(0, 160).c_str());
        }
        const Action& b = replay.turns[step][game.replayed];
        int sold0[2][N_ITEMS], inventory0[N_ITEMS];
        double revenue0[2];
        if (ledger)
            for (int k = 0; k < 2; ++k) {
                const Farm& f = sim.st.farms[k == 0 ? seat : game.replayed];
                revenue0[k] = f.sell_revenue;
                for (int i = 0; i < N_ITEMS; ++i) sold0[k][i] = f.sold_units[i], inventory0[i] = sim.st.market.inventory[i];
            }
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
        if (ledger) {
            int du[2][N_ITEMS];
            double estimate[2] = {0, 0}, price[N_ITEMS] = {};
            for (int i = 0; i < N_ITEMS; ++i) {
                for (int k = 0; k < 2; ++k) du[k][i] = sim.st.farms[k == 0 ? seat : game.replayed].sold_units[i] - sold0[k][i];
                if (const int n = du[0][i] + du[1][i]; n > 0) {
                    for (int j = 0; j < n; ++j) price[i] += market_price(i, inventory0[i] + j);
                    price[i] /= n;
                    for (int k = 0; k < 2; ++k) estimate[k] += du[k][i] * price[i];
                }
            }
            const int day = step / HOURS, hour = step % HOURS;
            for (int k = 0; k < 2; ++k) {
                const double actual = sim.st.farms[k == 0 ? seat : game.replayed].sell_revenue - revenue0[k];
                for (int i = 0; i < N_ITEMS; ++i)
                    if (du[k][i] > 0) {
                        r.units[ledger_index(k, day, i, hour)] += du[k][i];
                        r.revenue[ledger_index(k, day, i, hour)] += estimate[k] > 0 ? du[k][i] * price[i] * actual / estimate[k] : 0;
                    }
            }
        }
        shop_crn(sim, crn_shops);
        if (pin_shops && step + 1 < int(original.size()))
            for (int k = 0; k < sim.st.n_shops && k < original[step + 1].st.n_shops; ++k) sim.st.shops[k] = original[step + 1].st.shops[k];
        if (pin_weeds && step + 1 < int(original.size())) {
            Farm& f = sim.st.farms[game.replayed];
            const Farm& g = original[step + 1].st.farms[game.replayed];
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    Tile& t = f.tiles[y][x];
                    const TileKind want = TileKind(g.tiles[y][x].kind);
                    if ((t.kind == T_EMPTY || t.kind == T_WEED) && (want == T_EMPTY || want == T_WEED) && t.kind != want) {
                        t = Tile{};
                        t.kind = want;
                        const int index = y * BOARD + x;
                        const uint64_t bit = uint64_t{1} << (index & 63);
                        want == T_EMPTY ? f.empty_mask[index >> 6] |= bit : f.empty_mask[index >> 6] &= ~bit;
                    }
                }
        }
    }
    r.seed = replay.config.seed;
    r.first_shop = sim.st.n_shops > 0 ? sim.st.shops[0] : -1;
    r.own = sim.st.farms[seat].money;
    r.rival = sim.st.farms[game.replayed].money;
    r.original_rival = original.back().st.farms[game.replayed].money;
    r.original_other = original.back().st.farms[seat].money;
    return r;
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: replay_games list.txt threads out.csv\n";
        return 2;
    }
    std::vector<Game> games;
    std::ifstream list(argv[1]);
    for (std::string line; std::getline(list, line);) {  // trace replayed_seat [label]
        std::istringstream fields(line);
        Game g;
        if (fields >> g.trace >> g.replayed) games.push_back(g);
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
    out << "trace,replayed_seat,seed,own,rival,margin,original_rival,original_other,first_shop,geese_d4,cows_d4,sheep_d4,geese_d7,cows_d7,sheep_d7,geese_d10,cows_d10,sheep_d10,cash_d3,cash_d4,cash_d5,cash_d6,opp_div_day\n";
    int wins = 0;
    double margin = 0;
    for (size_t i = 0; i < games.size(); ++i) {
        const auto& r = results[i];
        out << games[i].trace << ',' << games[i].replayed << ',' << r.seed << ',' << r.own << ',' << r.rival << ','
            << r.own - r.rival << ',' << r.original_rival << ',' << r.original_other << ',' << r.first_shop;
        for (const int* n : {r.d4, r.d7, r.d10})
            for (int k = 0; k < 3; ++k) out << ',' << n[k];
        for (double c : r.cash) out << ',' << c;
        out << ',' << r.opp_div_day << '\n';
        wins += r.own > r.rival;
        margin += r.own - r.rival;
    }
    if (const char* path = std::getenv("REPLAY_LEDGER")) {  // trace,who,block,item,hour,units,revenue (nonzero cells)
        std::ofstream ledger(path);
        ledger << "trace,who,block,item,hour,units,revenue\n";
        for (size_t g = 0; g < games.size(); ++g)
            for (int k = 0; k < 2; ++k)
                for (int block = 0; block < 3; ++block)
                    for (int i = 0; i < N_ITEMS; ++i)
                        for (int h = 0; h < HOURS; ++h)
                            if (const size_t x = ledger_index(k, block * 10, i, h); results[g].units[x] > 0)
                                ledger << games[g].trace << ',' << k << ',' << block << ',' << i << ',' << h << ',' << results[g].units[x] << ','
                                       << results[g].revenue[x] << '\n';
    }
    std::cout << "games " << games.size() << " wins " << wins << " mean margin " << margin / games.size() << '\n';
    return 0;
}
