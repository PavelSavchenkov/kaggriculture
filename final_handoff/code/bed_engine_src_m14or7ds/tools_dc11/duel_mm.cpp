// duel_mm (m1 tree copy of sep29_mm_copy src/tools_dc11/duel_dc11.cpp, no DUEL_PLANT)
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
// DUEL_SELL=<file>: per hour, thin product (strawberry, milk, wool; DUEL_SELL_ALL=1: every product) and seat: shed stock at the turn,
// units sold, market inventory, units collected / harvested (made) and carried in pockets, for
// arm / opp (live) and real_arm / real_opp (the recording).
// DUEL_PLANT="mask first last": on those days the arm's new animals (mask 1) / new crops (2) / land (4) are the recorded seat's
// (its day label), the rest of the intent is the arm's own; tells whether the recorded player's plan volume reproduces its result.
// DUEL_REC="p p ...": the arm plays the recorded seat's actions, with the listed products' sales from our dc11 seller on the live state
// (the recording's deposits as incoming, as in seller_teacher); "" = the recording itself. Splits the recorded player's result into
// farm play vs selling, live against our reactive sub.
// DUEL_REC_DP_DAYS="first last" (with DUEL_REC): our seller only on those days.
// DUEL_REC_IDENTITY=1 (with DUEL_REC products): the recording's own lots through the replacement path (identity check, astra-001);
// =2: the recording's order list unchanged (checks the rest of the path).
// DUEL_REC_ORDER=inplace (default): a product's sale takes the slot of the recording's first sale of it, new products the leftover slots
// (passes the identity check); first: our sales before the recording's other orders; last: after them (the old placement).
// DUEL_REC_CAP=1: cap our lots at shed + this hour's recorded deposits (old); default no cap: the engine clips to the stock.
// DUEL_REC_UNTIL=X (with DUEL_REC): the recorded play ends at day X; the arm's own agent (fed the recorded history) plays from dawn X.
// DUEL_LOAN=1 (with DUEL_REC): the arm borrows whatever its recorded spend of a step needs and repays as soon as it has cash, so
// the recorded farm play never fails for cash; the final money is net of the debt.
// DUEL_PERTURB="day,hour,product,units": at that hour the arm sells that many extra units of the product (added to its own sale
// order, or a new order in slot 0; capped by its shed stock): a fork for causal response measurements (compare with the unperturbed
// run). DUEL_STOP_DAY=D: the game ends after day D (per-hour outputs up to then; final money is not meaningful).
// DUEL_MAX_ORDERS=N (probe): the engine accepts N orders per turn (hypothetical; with DC11_ORDERCAP=N the executor uses them): the
// upper bound of any dawn-slot scheme (dawn sales on top of the full hire wave); the arm seat keeps the real cap (its orders beyond
// it are truncated as the real engine did).
// DUEL_OPP_REC=1 (Imitation): the opponent seat replays the recording's own actions for that seat (the real team's play, open loop,
// cannot react), borrowing whatever a step's recorded spend needs (repaid as soon as it has cash; its money is reported net of the
// debt). Tests sale-timing changes against real selling hours instead of our lineage's.
// DUEL_OPP_SELL="p,p,..." (with DUEL_OPP_REC, Imitation): the replayed opponent's sales of the listed products come from its own agent's
// seller (the list's opponent model) on the live state, its farm stays the recording's: real production with a reacting seller.
// usage: duel_dc11 list.txt threads out.csv   (list line: trace arm_seat opponent_model.bin)
#include "agent/bc_overhaul/source/agent.hpp"
#include "source/convert.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <algorithm>
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
    std::vector<std::array<int, 4 * N_PRODUCTS>> prod[4];  // DUEL_PROD: per dawn cumulative produced / discarded / sold, stock (shed + pockets)
    std::vector<int> ops;  // DUEL_OPS: [who][day][kind: water, fertilize, feed, care][tile what] successful actions (state diffs)
    std::vector<std::array<int, 9>> sells;  // DUEL_SELL rows: who, day, hour, product, shed, sold, inventory, made (collected / harvested), pockets  // per dawn: geese, cows, sheep, field plants
    // [who: seat 0, seat 1 live, seat 0, seat 1 recorded][day][product, spend, hires]
    std::vector<double> units, revenue;
};
constexpr int N_COLS = N_PRODUCTS + 2;
constexpr const char* NAMES[N_COLS] = {"wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer", "spend", "hires"};
constexpr int N_BANDS = 4;  // hours 0-2, 3-11, 12-20, 21-23
inline int band_of(int hour) { return hour <= 2 ? 0 : hour <= 11 ? 1 : hour <= 20 ? 2 : 3; }
inline size_t cell(int who, int day, int band, int item) { return ((size_t(who) * 30 + day) * N_BANDS + band) * N_COLS + item; }

constexpr int N_WHAT = 32;
constexpr int N_KINDS = 5;  // water, fertilize, feed, care, plant
inline size_t op_cell(int who, int day, int kind, int what) { return ((size_t(who) * 30 + day) * N_KINDS + kind) * N_WHAT + what; }
// One step's successful water / fertilize / feed / care / planting per tile content, from the tile flags (before -> after) of both seats.
void count_ops(const Sim& before, const Sim& after, int who_base, Result& r) {
    const int day = std::min(29, before.st.step / HOURS);
    if (after.st.step / HOURS != day) return;  // the night update resets the flags
    for (int k = 0; k < 2; ++k)
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) {
                const Tile &a = before.st.farms[k].tiles[y][x], &b = after.st.farms[k].tiles[y][x];
                if (b.what >= N_WHAT) continue;
                if (!a.watered_today && b.watered_today) ++r.ops[op_cell(who_base + k, day, 0, b.what)];
                if (b.fertilized_until_day > a.fertilized_until_day) ++r.ops[op_cell(who_base + k, day, 1, b.what)];
                if (!a.fed_today && b.fed_today) ++r.ops[op_cell(who_base + k, day, 2, b.what)];
                if (!a.cared_today && b.cared_today) ++r.ops[op_cell(who_base + k, day, 3, b.what)];
                if (b.kind == T_PLANT && (a.kind != T_PLANT || a.planted_day != b.planted_day)) ++r.ops[op_cell(who_base + k, day, 4, b.what)];
            }
}

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
    Config config = replay.config;
    if (const char* m = std::getenv("DUEL_MAX_ORDERS")) config.max_orders = std::atoi(m);  // probe: a hypothetical engine order cap
    Sim sim(config);
    kag::agents::bc_overhaul::Agent agents[2];
    const std::string models[2] = {game.seat == 0 ? std::getenv("BC_OPUS_MODEL") : game.opponent,
                                   game.seat == 1 ? std::getenv("BC_OPUS_MODEL") : game.opponent};
    for (int s = 0; s < 2; ++s) {
        agents[s].model_path = models[s];
        const std::string sidecar = dc11_sidecar(models[s]);
        agents[s].options_text = !sidecar.empty() ? sidecar : "-";
        agents[s].reset(kag::agent::runtime::make_agent_init(sim, s));
    }
    if (const char* t = std::getenv("DUEL_PLANT")) {  // "mask first last": the recorded seat's daily new animals / crops / land
        int mask = 0, first = 0, last = 29;
        std::istringstream(t) >> mask >> first >> last;
        auto& arm = agents[game.seat];
        arm.plant_mask = mask, arm.plant_intent.assign(30, DayIntent{}), arm.plant_ok.assign(30, 0);
        for (int day = first; day <= last && (day + 1) * HOURS < int(original.size()); ++day) {
            const Conversion label = convert_day(original, replay.turns, game.seat, day);
            if (label.status != ConvertStatus::Ok) {
                std::fprintf(stderr, "plant %s d%d: no label\n", game.trace.c_str(), day);
                continue;
            }
            arm.plant_intent[day] = label.intent, arm.plant_ok[day] = 1;
        }
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
    const char* rec_env = std::getenv("DUEL_REC");
    bool dp[N_PRODUCTS]{};
    int n_dp = 0;
    if (rec_env) {  // product ids separated by spaces or commas
        std::string text = rec_env;
        std::replace(text.begin(), text.end(), ',', ' ');
        std::istringstream in(text);
        for (int p; in >> p;) dp[p] = true, ++n_dp;
    }
    dc11::DayMarket market_seat[2];
    bool dp_opp[N_PRODUCTS]{};  // DUEL_OPP_SELL
    int n_dp_opp = 0;
    if (const char* t = std::getenv("DUEL_OPP_SELL")) {
        std::string text = t;
        std::replace(text.begin(), text.end(), ',', ' ');
        std::istringstream in(text);
        for (int p; in >> p;) dp_opp[p] = true, ++n_dp_opp;
    }
    int perturb_step = -1, perturb_product = 0, perturb_units = 0;  // DUEL_PERTURB
    if (const char* t = std::getenv("DUEL_PERTURB")) {  // "day,hour,product,units" (commas or spaces)
        std::string text = t;
        std::replace(text.begin(), text.end(), ',', ' ');
        int day = 0, hour = 0;
        std::istringstream(text) >> day >> hour >> perturb_product >> perturb_units;
        perturb_step = day * HOURS + hour;
    }
    const int stop_day = std::getenv("DUEL_STOP_DAY") ? std::atoi(std::getenv("DUEL_STOP_DAY")) : 99;
    const bool loan = rec_env && std::getenv("DUEL_LOAN");
    const bool rec_identity = std::getenv("DUEL_REC_IDENTITY") != nullptr;
    const bool rec_identity_raw = rec_identity && std::string(std::getenv("DUEL_REC_IDENTITY")) == "2";
    const std::string rec_order = std::getenv("DUEL_REC_ORDER") ? std::getenv("DUEL_REC_ORDER") : "inplace";
    const bool rec_order_last = rec_order == "last", rec_order_inplace = rec_order == "inplace";
    const bool rec_nocap = !std::getenv("DUEL_REC_CAP");  // default: no stock cap (the engine clips); DUEL_REC_CAP=1: the old shed + deposit cap
    long rec_cut_units = 0, rec_cut_orders = 0;
    long id_steps = 0, id_capped = 0, id_merged = 0, id_not_first = 0;  // DUEL_REC_IDENTITY diagnostics  // our sales that did not fit the slots left by the recording's orders
    const int rec_until = std::getenv("DUEL_REC_UNTIL") ? std::atoi(std::getenv("DUEL_REC_UNTIL")) : 30;
    int dp_first = 0, dp_last = 29;
    if (const char* t = std::getenv("DUEL_REC_DP_DAYS")) std::istringstream(t) >> dp_first >> dp_last;
    double debt = 0, peak_debt = 0;
    const bool opp_rec = std::getenv("DUEL_OPP_REC") != nullptr;  // DUEL_OPP_REC
    double opp_debt = 0;
    std::vector<std::array<Action, 2>> turns;
    Result r;
    const bool days = std::getenv("DUEL_DAYS") != nullptr;
    const bool sell_log = std::getenv("DUEL_SELL") != nullptr;
    r.units.assign(4 * 30 * N_BANDS * N_COLS, 0), r.revenue.assign(4 * 30 * N_BANDS * N_COLS, 0);
    const bool ops = std::getenv("DUEL_OPS") != nullptr;
    if (ops) r.ops.assign(4 * 30 * N_KINDS * N_WHAT, 0);
    for (int step = 0; !sim.st.done && step < (stop_day + 1) * HOURS; ++step) {
        if (step % HOURS == 0)
            for (int s = 0; s < 2; ++s) {
                r.dawn[s].push_back(sim.st.farms[s].money - (s == game.seat ? debt : opp_debt));
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
                auto tally = [](const Farm& f) {
                    std::array<int, 4 * N_PRODUCTS> t{};
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        int stock = f.shed[p];
                        for (int u = 0; u < f.n_units; ++u) stock += f.inv[u][p];
                        t[p] = f.produced[p], t[N_PRODUCTS + p] = f.discarded[p], t[2 * N_PRODUCTS + p] = f.sold_units[p], t[3 * N_PRODUCTS + p] = stock;
                    }
                    return t;
                };
                r.prod[s].push_back(tally(sim.st.farms[s]));
                if (step < int(original.size())) r.prod[2 + s].push_back(tally(original[step].st.farms[s]));
                r.real_herd[s].push_back(step < int(original.size()) ? count(original[step].st.farms[s]) : std::array<int, 4>{-1, -1, -1, -1});
            }
        Action a[2];
        agent::AgentObservation rec_obs, rec_obs_seat[2];
        for (int s = 0; s < 2; ++s) {
            const auto obs = kag::agent::runtime::make_observation(sim, s);
            if (opp_rec && s != game.seat && !n_dp_opp) {  // DUEL_OPP_REC: the recorded opponent
                a[s] = step < int(replay.turns.size()) ? replay.turns[step][s] : Action{};
                continue;
            }
            if (!(opp_rec && s != game.seat) && (!rec_env || s != game.seat || step / HOURS >= rec_until)) {
                agents[s].act(obs, budget, a[s]);
                continue;
            }
            // DUEL_REC: the recorded seat's action; the listed products' sales from our seller on the live state (as in seller_teacher:
            // the recording's deposits that day as incoming, the night room the recording left, no reserves for the listed products)
            auto& arm = agents[s];
            arm.seller_advance(obs);
            const int day = step / HOURS, hour = step % HOURS;
            if (hour == 0) market_seat[s] = arm.seller_market(obs);
            const bool* dpm = s == game.seat ? dp : dp_opp;  // DUEL_OPP_SELL: the opponent's product mask
            const int ndp = s == game.seat ? n_dp : n_dp_opp, dfirst = s == game.seat ? dp_first : 0, dlast = s == game.seat ? dp_last : 29;
            a[s] = step < int(replay.turns.size()) ? replay.turns[step][s] : Action{};
            if (ndp && day >= dfirst && day <= dlast && (day + 1) * HOURS < int(original.size())) {
                int incoming[HOURS][N_PRODUCTS]{};
                for (int p = 0; p < N_PRODUCTS; ++p) {
                    if (!dpm[p]) continue;
                    int cum = 0;
                    for (int h = 0; h < HOURS; ++h) {
                        const int j = day * HOURS + h;
                        const Farm &f0 = original[j].st.farms[s], &f1 = original[j + 1].st.farms[s];
                        const int sold = f1.sold_units[p] - f0.sold_units[p];
                        cum += h == HOURS - 1 ? std::max(0, sold - int(f0.shed[p])) : std::max(0, int(f1.shed[p]) - int(f0.shed[p]) + sold);
                        incoming[h][p] = cum;
                    }
                }
                int room = 0;
                if ((day + 1) * HOURS + 1 < int(original.size())) {
                    const Farm &next = original[(day + 1) * HOURS].st.farms[s], &h23 = original[(day + 1) * HOURS - 1].st.farms[s];
                    int other = 0, pockets = 0;
                    for (int i = 0; i < N_ITEMS; ++i)
                        if (i >= N_PRODUCTS || !dpm[i]) other += next.shed[i];
                    for (int p = 0; p < N_PRODUCTS; ++p)
                        if (dpm[p]) pockets += std::max(0, int(next.shed[p]) - std::max(0, int(h23.shed[p]) - (next.sold_units[p] - h23.sold_units[p])));
                    room = std::max(0, 100 - other - pockets - 2);
                }
                dc11::DayMarket m = market_seat[s];
                arm.seller_hour(obs, m);
                int reserve[N_PRODUCTS]{}, sell[N_PRODUCTS]{};
                for (int i = 0; i < N_PRODUCTS; ++i) reserve[i] = dpm[i] ? 0 : 1 << 20;
                double charge = 0;
                bool flag[N_PRODUCTS];
                const bool timing = arm.model_flags(obs, flag);  // sellmodel=3 / 4: the model's hours
                dc11::choose_sales(obs, incoming, HOURS, m, reserve, room, sell, &charge, nullptr, timing && arm.model_mode() == 3 ? flag : nullptr);
                arm.model_sales(obs, sell, charge > 0);  // sellmodel=1 / 2 (I1): no-op without it
                if (timing && charge == 0)
                    for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)})
                        if (!flag[p]) sell[p] = 0;
                Action& x = a[s];
                // DUEL_REC_IDENTITY=1 (astra-001): the recording's own lots of the listed products go through this path; with the order
                // placement below it must reproduce the recording exactly (checks that the harness alone changes nothing).
                if (rec_identity) {
                    for (int p = 0; p < N_PRODUCTS; ++p) sell[p] = 0;
                    for (int j = 0; j < x.n_orders; ++j)
                        if (x.orders[j].op == M_SELL && x.orders[j].item < N_PRODUCTS && dpm[x.orders[j].item]) sell[x.orders[j].item] += x.orders[j].n;
                }
                arm.model_record(obs, sell);
                Action kept{};
                for (int j = 0; j < x.n_orders; ++j)
                    if (!(x.orders[j].op == M_SELL && x.orders[j].item < N_PRODUCTS && dpm[x.orders[j].item])) kept.orders[kept.n_orders++] = x.orders[j];
                // Our sales get the slots the recording's other orders leave (so every recorded hire / purchase is still sent: the farm
                // stays the recording's). Order position (astra-001): sales first, as M&M and our live executor place them
                // (DUEL_REC_ORDER=last: after the recording's orders, the old placement).
                Action sales{};
                for (int p = 0; p < N_PRODUCTS; ++p) {  // worker actions run before market orders: this hour's deposits are sellable
                    const int deposit = incoming[hour][p] - (hour ? incoming[hour - 1][p] : 0);
                    const int n = rec_nocap ? sell[p] : std::min<int>(sell[p], sim.st.farms[s].shed[p] + deposit);
                    if (!dpm[p] || n <= 0) continue;
                    if (kept.n_orders + sales.n_orders < 10) sales.orders[sales.n_orders++] = {M_SELL, uint8_t(p), n};
                    else rec_cut_units += n, ++rec_cut_orders;
                }
                if (rec_identity) {  // why the rebuilt list differs from the recording (identity diagnostics)
                    const Action& orig = x;
                    int orig_units[N_PRODUCTS]{}, new_units[N_PRODUCTS]{}, orig_sell_orders = 0, first_sell = -1;
                    for (int j = 0; j < orig.n_orders; ++j)
                        if (orig.orders[j].op == M_SELL && orig.orders[j].item < N_PRODUCTS && dpm[orig.orders[j].item]) {
                            orig_units[orig.orders[j].item] += orig.orders[j].n, ++orig_sell_orders;
                            if (first_sell < 0) first_sell = j;
                        }
                    for (int j = 0; j < sales.n_orders; ++j) new_units[sales.orders[j].item] += sales.orders[j].n;
                    bool capped = false;
                    for (int p = 0; p < N_PRODUCTS; ++p) capped |= new_units[p] < orig_units[p];
                    id_steps += orig_sell_orders > 0;
                    id_capped += capped;
                    id_merged += orig_sell_orders > sales.n_orders;
                    id_not_first += orig_sell_orders > 0 && first_sell > 0;
                    if (capped && id_capped <= 5)
                        for (int p = 0; p < N_PRODUCTS; ++p)
                            if (new_units[p] < orig_units[p])
                                std::fprintf(stderr, "idcap d%d h%d p%d rec %d emitted %d shed %d deposit %d\n", day, hour, p, orig_units[p], new_units[p],
                                             int(sim.st.farms[s].shed[p]), incoming[hour][p] - (hour ? incoming[hour - 1][p] : 0));
                }
                if (rec_identity_raw) {
                    // DUEL_REC_IDENTITY=2: the recording's order list unchanged (tests the rest of this path: seller calls, loans)
                } else if (rec_order_inplace) {
                    Action out = x;  // keeps the recording's worker actions; only the order list is rebuilt
                    out.n_orders = 0;
                    bool placed[N_PRODUCTS]{};
                    for (int j = 0; j < x.n_orders; ++j) {
                        const auto& o = x.orders[j];
                        if (o.op == M_SELL && o.item < N_PRODUCTS && dpm[o.item]) {  // the recording's sale slot: ours for that product, once
                            if (placed[o.item]) continue;
                            placed[o.item] = true;
                            for (int k = 0; k < sales.n_orders; ++k)
                                if (sales.orders[k].item == o.item) out.orders[out.n_orders++] = sales.orders[k];
                        } else out.orders[out.n_orders++] = o;
                    }
                    for (int k = 0; k < sales.n_orders; ++k)  // products the recording did not sell this hour: leftover slots
                        if (!placed[sales.orders[k].item] && out.n_orders < 10) out.orders[out.n_orders++] = sales.orders[k];
                    x = out;
                } else {
                    x.n_orders = 0;
                    const Action& front = rec_order_last ? kept : sales;
                    const Action& back = rec_order_last ? sales : kept;
                    for (int j = 0; j < front.n_orders; ++j) x.orders[x.n_orders++] = front.orders[j];
                    for (int j = 0; j < back.n_orders; ++j) x.orders[x.n_orders++] = back.orders[j];
                }
            }
            rec_obs = obs;
            rec_obs_seat[s] = obs;
        }
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
        if (step == perturb_step) {  // DUEL_PERTURB: the arm sells extra units first (slot 0), capped by its shed stock
            Action& x = a[game.seat];
            const int p = perturb_product;
            int planned = 0, slot = -1;
            for (int j = 0; j < x.n_orders; ++j)
                if (x.orders[j].op == M_SELL && x.orders[j].item == p) planned += x.orders[j].n, slot = j;
            const int extra = std::max(0, std::min(perturb_units, int(sim.st.farms[game.seat].shed[p]) - planned));
            if (extra > 0) {
                if (slot >= 0) x.orders[slot].n += extra;
                else if (x.n_orders < 10) {
                    for (int j = x.n_orders; j > 0; --j) x.orders[j] = x.orders[j - 1];
                    x.orders[0] = {M_SELL, uint8_t(p), extra}, ++x.n_orders;
                }
            }
            std::fprintf(stderr, "perturb %s step %d product %d extra %d (asked %d, planned %d)\n", game.trace.c_str(), step, p, extra, perturb_units, planned);
        }
        if (std::getenv("DUEL_MAX_ORDERS")) {  // probe: the larger cap is for the live seat only; the arm keeps the real engine's cap
            Action& x = a[game.seat];
            x.n_orders = std::min(x.n_orders, replay.config.max_orders);
        }
        turns.push_back({a[0], a[1]});
        if (rec_env && step / HOURS < rec_until) agents[game.seat].seller_record(rec_obs_seat[game.seat], a[game.seat]);
        if (opp_rec && n_dp_opp) agents[1 - game.seat].seller_record(rec_obs_seat[1 - game.seat], a[1 - game.seat]);  // DUEL_OPP_SELL
        if (loan && step / HOURS < rec_until && step + 1 < int(original.size())) {  // DUEL_LOAN: borrow when a trial step spends less than the recording did
            const int s = game.seat;
            const auto need = original[step + 1].st.farms[s].total_spend - original[step].st.farms[s].total_spend;
            for (int tries = 0; tries < 4; ++tries) {
                Sim trial = sim;
                trial.step(a[0], a[1]);
                const auto got = trial.st.farms[s].total_spend - sim.st.farms[s].total_spend;
                if (got >= need) break;
                const auto add = tries < 3 ? need - got : need;
                sim.st.farms[s].money += add, debt += add, peak_debt = std::max(peak_debt, debt);
            }
        }
        if (opp_rec && step + 1 < int(original.size())) {  // DUEL_OPP_REC loans: the recorded opponent's spend never fails for cash
            const int s = 1 - game.seat;
            const auto need = original[step + 1].st.farms[s].total_spend - original[step].st.farms[s].total_spend;
            for (int tries = 0; tries < 4; ++tries) {
                Sim trial = sim;
                trial.step(a[0], a[1]);
                const auto got = trial.st.farms[s].total_spend - sim.st.farms[s].total_spend;
                if (got >= need) break;
                const auto add = tries < 3 ? need - got : need;
                sim.st.farms[s].money += add, opp_debt += add;
            }
        }
        const Sim before = days || sell_log || ops ? sim : Sim(replay.config);
        sim.step(a[0], a[1]);
        if (opp_rec) {  // repay the opponent's loans as soon as it has cash
            Farm& f = sim.st.farms[1 - game.seat];
            const auto back = std::min<decltype(f.money)>(decltype(f.money)(opp_debt), std::max<decltype(f.money)>(0, f.money));
            f.money -= back, opp_debt -= back;
        }
        if (loan) {  // repay as soon as there is cash
            Farm& f = sim.st.farms[game.seat];
            const auto back = std::min<decltype(f.money)>(decltype(f.money)(debt), std::max<decltype(f.money)>(0, f.money));
            f.money -= back, debt -= back;
        }
        if (sell_log) {
            auto pockets = [](const Farm& f, int p) {
                int n = 0;
                for (int u = 0; u < f.n_units; ++u) n += f.inv[u][p];
                return n;
            };
            static const bool all = std::getenv("DUEL_SELL_ALL") != nullptr;  // every product, not only strawberry / milk / wool
            for (int prod = 0; prod < N_PRODUCTS; ++prod) {
                if (!all && prod != STRAWBERRY && prod != MILK && prod != WOOL) continue;
                for (int s = 0; s < 2; ++s)
                    r.sells.push_back({s, step / HOURS, step % HOURS, prod, int(before.st.farms[s].shed[prod]),
                                       sim.st.farms[s].sold_units[prod] - before.st.farms[s].sold_units[prod], before.st.market.inventory[prod],
                                       sim.st.farms[s].produced[prod] - before.st.farms[s].produced[prod], pockets(before.st.farms[s], prod)});
                if (step + 1 < int(original.size()))
                    for (int s = 0; s < 2; ++s)
                        r.sells.push_back({2 + s, step / HOURS, step % HOURS, prod, int(original[step].st.farms[s].shed[prod]),
                                           original[step + 1].st.farms[s].sold_units[prod] - original[step].st.farms[s].sold_units[prod],
                                           original[step].st.market.inventory[prod],
                                           original[step + 1].st.farms[s].produced[prod] - original[step].st.farms[s].produced[prod],
                                           pockets(original[step].st.farms[s], prod)});
            }
        }
        if (ops) {
            count_ops(before, sim, 0, r);
            if (step + 1 < int(original.size())) count_ops(original[step], original[step + 1], 2, r);
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
    if (agents[game.seat].plant_mask)
        std::fprintf(stderr, "plant %s: applied %d days, invalid %d\n", game.trace.c_str(), agents[game.seat].plant_used, agents[game.seat].plant_failed);
    for (int s = 0; s < 2; ++s) r.money[s] = sim.st.farms[s].money, r.real[s] = original.back().st.farms[s].money;
    if (opp_rec) r.money[1 - game.seat] -= opp_debt;  // DUEL_OPP_REC: net of the opponent's outstanding debt
    r.money[game.seat] -= debt;
    if (loan) std::fprintf(stderr, "loan %s: debt at the end %.0f, peak %.0f\n", game.trace.c_str(), debt, peak_debt);
    if (rec_env && n_dp) std::fprintf(stderr, "recsell %s: cut %ld units in %ld orders (no free slot)\n", game.trace.c_str(), rec_cut_units, rec_cut_orders);
    if (rec_identity) std::fprintf(stderr, "identity %s: sell steps %ld, capped %ld, merged %ld, sells not first %ld\n", game.trace.c_str(), id_steps, id_capped, id_merged, id_not_first);
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
        out << "trace,who,day,hour,product,shed,sold,inv,made,pockets\n";
        for (size_t g = 0; g < games.size(); ++g) {
            const int a = games[g].seat;
            for (const auto& x : results[g].sells) {
                const bool live = x[0] < 2, arm = (x[0] % 2) == a;
                out << games[g].trace << "," << (live ? (arm ? "arm" : "opp") : (arm ? "real_arm" : "real_opp")) << "," << x[1] << "," << x[2] << ","
                    << x[3] << "," << x[4] << "," << x[5] << "," << x[6] << "," << x[7] << "," << x[8] << "\n";
            }
        }
    }
    if (const char* path = std::getenv("DUEL_OPS")) {  // per day: successful water / fertilize / feed / care by tile content
        std::ofstream out(path);
        out << "trace,who,day,kind,what,n\n";
        const char* who[4] = {"arm", "opp", "real_arm", "real_opp"};
        const char* kinds[N_KINDS] = {"water", "fertilize", "feed", "care", "plant"};
        for (size_t i = 0; i < games.size(); ++i)
            for (int w = 0; w < 4; ++w) {
                const int seat = w % 2 == 0 ? games[i].seat : 1 - games[i].seat;
                for (int d = 0; d < 30; ++d)
                    for (int k = 0; k < N_KINDS; ++k)
                        for (int t = 0; t < N_WHAT; ++t)
                            if (const int n = results[i].ops[op_cell((w / 2) * 2 + seat, d, k, t)])
                                out << games[i].trace << ',' << who[w] << ',' << d << ',' << kinds[k] << ',' << t << ',' << n << '\n';
            }
    }
    if (const char* path = std::getenv("DUEL_PROD")) {  // per dawn and product: cumulative produced / discarded / sold, stock
        std::ofstream out(path);
        out << "trace,who,day,product,produced,discarded,sold,stock\n";
        const char* who[4] = {"arm", "opp", "real_arm", "real_opp"};
        for (size_t i = 0; i < games.size(); ++i)
            for (int w = 0; w < 4; ++w) {
                const int seat = w % 2 == 0 ? games[i].seat : 1 - games[i].seat;
                const auto& rows = results[i].prod[(w / 2) * 2 + seat];
                for (size_t d = 0; d < rows.size(); ++d)
                    for (int p = 0; p < N_PRODUCTS; ++p)
                        out << games[i].trace << ',' << who[w] << ',' << d << ',' << p << ',' << rows[d][p] << ',' << rows[d][N_PRODUCTS + p] << ','
                            << rows[d][2 * N_PRODUCTS + p] << ',' << rows[d][3 * N_PRODUCTS + p] << '\n';
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
