// Full games: bc_opus against an opponent (agent_sep23 or pass), both seats per seed.
// BC_PERTURB=p: on each of our dawns up to day 12, with probability p one decision push (as in
// tools/search_games) replaces today's decoding (on-policy value-model data with varied decisions).
// FULL_BUDGET=f: Kaggle time rules for our seat (1 s per step + 60 s overage) on a machine f times slower;
// the agent sees the remaining overage and its deadline is scaled (DC10_TIME_SCALE). FULL_BUDGET_BOTH=1: a bc:
// opponent too.
// Opponents: pass, agent_sep23, a registry name, bc:<model.bin>, dc11:<model.bin> (full_games_dc11 only).
// usage: full_games opponent seed_start games threads out.csv
#include "agent/bc_opus/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "opponents/sep23_adapter.hpp"
#include "opponents/opponent.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <type_traits>
#include "tools/shop_crn.hpp"

using namespace dc10;

#ifdef WITH_DC11
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
}  // namespace
#endif
#ifdef MINE_DC11
namespace {
// full_games_dc11: our seat plays the dc11 day compiler (vendored, dc11/VENDORED.md) with the model in
// BC_OPUS_MODEL and options from DC11_OPTIONS (else <model>.dc11); dc11 caps its work by evaluations (no time hooks).
struct Mine : kag::agents::bc_overhaul::Agent {
    kag::agents::bc_opus::DecodeKnobs knobs;  // BC_PERTURB is not supported
    Mine() {
        if (const char* m = std::getenv("BC_OPUS_MODEL")) model_path = m;
        if (!std::getenv("DC11_OPTIONS")) options_text = dc11_sidecar(model_path);  // "" -> dc11 defaults
    }
    void set_time_left(double) {}
};
}  // namespace
#else
using Mine = kag::agents::bc_opus::Agent;
#endif

namespace {
struct Result {
    uint64_t seed = 0;
    int seat = 0;
    double own = 0, rival = 0;
    int uncompiled = 0, fallback = 0, invalid = 0, discarded = 0, rival_discarded = 0;
    double compile_max = 0, compile_total = 0;
    int land = 0, rival_land = 0, animals = 0, rival_animals = 0;
    int escaped = 0, rival_escaped = 0;  // animals lost (only escapes remove a placed animal)
    int animals_d4 = -1, rival_animals_d4 = -1;  // placed animals at dawn of day 4
    double overage_left = 60, overage_step = 0;  // FULL_BUDGET: Kaggle overage left at the end, largest one-step use
};

int placed_animals(const Farm& farm) {
    int n = 0;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) n += farm.tiles[y][x].has_animal;
    return n;
}

struct OtherBC;

template <class Opponent>
Result play(uint64_t seed, int seat) {
    Config config;
    config.seed = seed;
    Sim sim(config);
    int crn_shops = 0;  // SHOP_CRN: shops fixed so far
    Mine mine;
    mine.reset(kag::agent::runtime::make_agent_init(sim, seat));
    Opponent other;
    other.reset(kag::agent::runtime::make_agent_init(sim, 1 - seat));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    const char* key = std::getenv("BC_GAME_TRACE");
    const bool trace = key && std::to_string(seed) + ":" + std::to_string(seat) == key;
    std::vector<std::array<Action, 2>> turns;  // BC_WRITE_TRACES=<dir>: the game as a trace
    const char* perturb = std::getenv("BC_PERTURB");
    std::mt19937_64 rng(seed * 2 + seat);
    const kag::agents::bc_opus::DecodeKnobs plain = mine.knobs;
    int escaped = 0, rival_escaped = 0;
    int animals_d4 = -1, rival_animals_d4 = -1;
    int loss_units = 0, loss_orders = 0, loss_weeds = 0, loss_weeds_dry = 0, loss_weeds_decay = 0, loss_cap = 0;  // FULL_EVENTS counters
    // FULL_BUDGET=f: Kaggle time rules for our seat on a machine f times slower (1 s per step, 60 s overage).
    const double slow = std::getenv("FULL_BUDGET") ? std::atof(std::getenv("FULL_BUDGET")) : 0;
    const double deadline_k = std::getenv("FULL_DEADLINE") ? std::atof(std::getenv("FULL_DEADLINE")) : 0;
    const bool dawnlog = std::getenv("FULL_DAWNLOG");
    double overage = 60, overage_step = 0;
    // FULL_BUDGET_BOTH=1: a bc: opponent plays under the same time rules.
    [[maybe_unused]] const bool both = slow && std::getenv("FULL_BUDGET_BOTH");
    [[maybe_unused]] double other_overage = 60;
    while (!sim.st.done) {
        Action a, b;
        if (perturb && sim.st.step % HOURS == 0) {
            mine.knobs = plain;
            if (sim.st.day <= 12 && std::uniform_real_distribution<>(0, 1)(rng) < std::atof(perturb)) {
                switch (std::uniform_int_distribution<>(0, 5)(rng)) {
                    case 0: mine.knobs.q_crop = 0.3; break;
                    case 1: mine.knobs.q_crop = 0.7; break;
                    case 2: mine.knobs.q_animal = 0.3; break;
                    case 3: mine.knobs.q_animal = 0.7; break;
                    case 4: mine.knobs.land_bias = 3; break;
                    default: mine.knobs.land_bias = -3;
                }
            }
        }
        const auto started = std::chrono::steady_clock::now();
        if (slow) mine.set_time_left(overage);
        // FULL_DEADLINE=1 (with FULL_BUDGET=f): the Kaggle bridge's soft deadline (lb_bridge.cpp: 0.9 s + overage left / days left),
        // scaled to this machine; FULL_DEADLINE=k scales the Kaggle deadline by k. FULL_DAWNLOG=1: one stderr line per dawn.
        kag::agent::DecisionBudget mine_budget = budget;
        const double deadline_s = slow && deadline_k > 0 ? deadline_k * (0.9 + std::max(0.0, overage) / std::max(1, LAST_DAY + 1 - sim.st.day)) : 0;
        if (deadline_s > 0) mine_budget.soft_deadline = started + std::chrono::microseconds(int64_t(1e6 * deadline_s / slow));
        mine.act(kag::agent::runtime::make_observation(sim, seat), mine_budget, a);
        if (slow) {
            const double step_s = slow * std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            const double used = std::max(0.0, step_s - 1.0);
            overage -= used, overage_step = std::max(overage_step, used);
            if (dawnlog && sim.st.step % HOURS == 0)
                std::fprintf(stderr, "dawn %llu %d %d %.3f %.3f %.2f\n", (unsigned long long)seed, seat, sim.st.day, step_s, deadline_s, overage);
        }
#ifndef MINE_DC11
        if (trace && sim.st.step % HOURS == 0) {
            const Farm &f = sim.st.farms[seat], &g = sim.st.farms[1 - seat];
            int plants[N_CROPS]{}, animals[N_ANIMALS]{}, rp[N_CROPS]{}, ra[N_ANIMALS]{};
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    const Tile &t = f.tiles[y][x], &u = g.tiles[y][x];
                    if (t.kind == T_PLANT) ++plants[t.what];
                    if (t.has_animal) ++animals[t.what - GOOSE];
                    if (u.kind == T_PLANT) ++rp[u.what];
                    if (u.has_animal) ++ra[u.what - GOOSE];
                }
            const auto& r = mine.reports().back();
            std::fprintf(stderr, "d%02d cash %7.0f/%7.0f land %d/%d plants %d %d %d %d %d vs %d %d %d %d %d animals %d %d %d vs %d %d %d | status %d fb %d %.0fms\n  intent: %s\n",
                         sim.st.day, f.money, g.money, f.n_quadrants, g.n_quadrants, plants[0], plants[1], plants[2], plants[3],
                         plants[4], rp[0], rp[1], rp[2], rp[3], rp[4], animals[0], animals[1], animals[2], ra[0], ra[1], ra[2],
                         r.status, r.fallback, r.compile_ms,
                         kag::agents::bc_opus::summarize(mine.last_intent(), mine.last_schema()).c_str());
            if (r.fallback || r.status) std::fprintf(stderr, "  reason: %s\n", r.reason.substr(0, 700).c_str());
        }
#endif
        if constexpr (std::is_same_v<Opponent, OtherBC>) {
            const auto other_started = std::chrono::steady_clock::now();
            if (both) other.agent.set_time_left(other_overage);
            other.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, b);
            if (both)
                other_overage -= std::max(0.0, slow * std::chrono::duration<double>(std::chrono::steady_clock::now() - other_started).count() - 1.0);
        } else {
            other.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, b);
        }
        turns.push_back(seat == 0 ? std::array<Action, 2>{a, b} : std::array<Action, 2>{b, a});
        const int before = placed_animals(sim.st.farms[seat]), rival_before = placed_animals(sim.st.farms[1 - seat]);
        int discarded_before[N_ITEMS];
        for (int i = 0; i < N_ITEMS; ++i) discarded_before[i] = sim.st.farms[seat].discarded[i];
        if (sim.st.day == 4 && animals_d4 < 0) animals_d4 = before, rival_animals_d4 = rival_before;
        const int step_before = sim.st.step;
        if (std::getenv("FULL_EVENTS")) {  // failed unit actions / order units of our seat this step, and plants that become weeds tonight
            const auto d = (seat == 0 ? sim.diagnose_joint_actions(a, b) : sim.diagnose_joint_actions(b, a)).players[seat];
            loss_units += d.requested_unit_actions - d.successful_unit_actions;
            loss_orders += d.requested_order_units - d.successful_order_units;
            if (d.requested_order_units != d.successful_order_units) {  // orderfail,seed,seat,day,hour,money | op/item/n ...
                std::string o;
                for (int k = 0; k < a.n_orders; ++k) o += " " + std::to_string(a.orders[k].op) + "/" + std::to_string(a.orders[k].item) + "/" + std::to_string(a.orders[k].n);
                std::fprintf(stderr, "orderfail,%d,%d,%d,%d,%.0f |%s\n", int(seed), seat, sim.st.step / HOURS, sim.st.step % HOURS, sim.st.farms[seat].money, o.c_str());
            }
        }
        bool was_plant[BOARD][BOARD]{};
        Tile night_before[BOARD][BOARD];
        if (std::getenv("FULL_EVENTS") && sim.st.step % HOURS == HOURS - 1)
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) was_plant[y][x] = sim.st.farms[seat].tiles[y][x].kind == T_PLANT, night_before[y][x] = sim.st.farms[seat].tiles[y][x];
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
        if (std::getenv("FULL_EVENTS") && step_before % HOURS == HOURS - 1) {
            const int night_day = step_before / HOURS;
            for (int y = 0; y < BOARD; ++y)  // our plants of last step that are weeds now (dry twice, or decayed to 0)
                for (int x = 0; x < BOARD; ++x) {
                    const Tile &b0 = night_before[y][x], &a0 = sim.st.farms[seat].tiles[y][x];
                    if (was_plant[y][x] && a0.kind == T_WEED) {
                        ++loss_weeds;
                        (b0.consecutive_dry >= 1 && !b0.watered_today ? loss_weeds_dry : loss_weeds_decay) += 1;
                        std::fprintf(stderr, "weedev,%d,%d,%d,%d,%d,%d,%d,%d\n", int(seed), seat, night_day, int(b0.what), night_day - b0.planted_day, int(b0.yield_units),
                                     int(b0.consecutive_dry), int(b0.watered_today));
                    }
                    if (b0.has_animal && a0.has_animal) {  // production lost to the held cap tonight
                        const auto& def = ANIMALS[b0.what - GOOSE];
                        const int age = night_day - b0.planted_day + 1;
                        if (b0.yield_units >= def.max_held && age >= def.first_yield_day && (age - def.first_yield_day) % def.interval == 0) ++loss_cap;
                    }
                }
        }
        shop_crn(sim, crn_shops);
#ifdef MINE_DC11
        if (std::getenv("FULL_EVENTS") && step_before % HOURS == 0 && !mine.reports().empty()) {  // dawnev,seed,seat,day,animals,feeds,fallback,trims,dropped,reason
            const auto& r = mine.reports().back();
            std::fprintf(stderr, "dawnev,%d,%d,%d,%d,%d,%d,%d,%d,%s |%s\n", seed, seat, step_before / HOURS, r.animals, r.feed_intent, r.fallback, r.trims, r.dropped,
                         r.reason.substr(0, 300).c_str(), r.groups.c_str());
            if (r.asked[0] + r.asked[1] + r.asked[2] > 0)  // animalev,seed,seat,day,asked g c s,planned g c s,fallback,trims,reason
                std::fprintf(stderr, "animalev,%d,%d,%d,%d %d %d,%d %d %d,%d,%d,%s\n", seed, seat, step_before / HOURS, r.asked[0], r.asked[1], r.asked[2],
                             r.planned[0], r.planned[1], r.planned[2], r.fallback, r.trims, r.reason.substr(0, 400).c_str());
        }
#endif
        if (std::getenv("FULL_EVENTS")) {  // event,seed,seat,day,hour,escaped,discarded item:n ... (our farm's losses this step)
            const int lost = std::max(0, before - placed_animals(sim.st.farms[seat]));
            std::string d;
            for (int i = 0; i < N_ITEMS; ++i)
                if (const int n = sim.st.farms[seat].discarded[i] - discarded_before[i]; n > 0) d += " " + std::to_string(i) + ":" + std::to_string(n);
            if (lost || !d.empty())
                std::fprintf(stderr, "event,%d,%d,%d,%d,%d,%s\n", seed, seat, step_before / HOURS, step_before % HOURS, lost, d.c_str());
        }
        if (sim.st.day < 25) {  // before day 25 (later, unfed animals are often left to escape: no sale left)
            escaped += std::max(0, before - placed_animals(sim.st.farms[seat]));
            rival_escaped += std::max(0, rival_before - placed_animals(sim.st.farms[1 - seat]));
        }
    }
    if (std::getenv("FULL_EVENTS"))  // lossstat,seed,seat,failed_unit_actions,failed_order_units,new_weeds,dry,decay,cap_lost
        std::fprintf(stderr, "lossstat,%d,%d,%d,%d,%d,%d,%d,%d\n", int(seed), seat, loss_units, loss_orders, loss_weeds, loss_weeds_dry, loss_weeds_decay, loss_cap);
    if (const char* dir = std::getenv("BC_WRITE_TRACES"))
        save_replay(std::string(dir) + "/" + std::to_string(seed) + "_" + std::to_string(seat) + ".txt", config, turns);
    Result r;
    r.seed = seed;
    r.seat = seat;
    const Farm &f = sim.st.farms[seat], &g = sim.st.farms[1 - seat];
    r.own = f.money, r.rival = g.money;
    for (const auto& d : mine.reports()) {
        r.uncompiled += d.status != int(CompileStatus::Ok);
        r.fallback += d.status == int(CompileStatus::Ok) && d.fallback != 0;
#ifndef MINE_DC11
        r.invalid += !d.invalid.empty();
#endif
        r.compile_max = std::max(r.compile_max, d.compile_ms);
        r.compile_total += d.compile_ms;
    }
    for (int i = 0; i < N_ITEMS; ++i) r.discarded += f.discarded[i], r.rival_discarded += g.discarded[i];
    r.land = f.n_quadrants, r.rival_land = g.n_quadrants;
    r.escaped = escaped, r.rival_escaped = rival_escaped;
    r.animals_d4 = animals_d4, r.rival_animals_d4 = rival_animals_d4;
    r.overage_left = overage, r.overage_step = overage_step;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) r.animals += f.tiles[y][x].has_animal, r.rival_animals += g.tiles[y][x].has_animal;
    return r;
}

// A registered in-house opponent chosen by name (BC_OPPONENT) at construction.
struct Inhouse {
    std::unique_ptr<opponents::Opponent> agent = opponents::make(std::getenv("BC_OPPONENT"));
    void reset(const kag::agent::AgentInit& init) { agent->reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, Action& a) { agent->act(o, b, a); }
};

// Another bc_opus model as the opponent ("bc:<model.bin>", from BC_OPPONENT_MODEL).
struct OtherBC {
    kag::agents::bc_opus::Agent agent;
    OtherBC() {
        agent.knobs = kag::agents::bc_opus::DecodeKnobs{};
        agent.model_path = std::getenv("BC_OPPONENT_MODEL");
        agent.opening_days = kag::agents::bc_opus::opening_experiment() ? 0 : -1;  // BC_OPENING_* is for the agent under test
    }
    void reset(const kag::agent::AgentInit& init) { agent.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, Action& a) { agent.act(o, b, a); }
};

#ifdef WITH_DC11
// Opponent "dc11:<model.bin>": the model with the dc11 compiler at its defaults (BC_OPPONENT_DC11_OPTIONS overrides).
struct OtherDC11 {
    kag::agents::bc_overhaul::Agent agent;
    OtherDC11() {
        agent.model_path = std::getenv("BC_OPPONENT_MODEL");
        const char* options = std::getenv("BC_OPPONENT_DC11_OPTIONS");  // e.g. "interleave=0" (default: dc11 defaults)
        const std::string sidecar = dc11_sidecar(agent.model_path);
        agent.options_text = options && *options ? options : !sidecar.empty() ? sidecar : "-";
    }
    void reset(const kag::agent::AgentInit& init) { agent.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, Action& a) { agent.act(o, b, a); }
};
#endif

struct Pass {
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget&, Action& action) {
        action.clear();
        action.n_units = 1 + o.own_hand_count();
        for (int u = 0; u < action.n_units; ++u) action.units[u] = UnitAction{};
        action.finalize();
    }
};
}

int main(int argc, char** argv) {
    if (const char* slow = std::getenv("FULL_BUDGET")) setenv("DC10_TIME_SCALE", slow, 1);  // Kaggle time rules
    else setenv("DC10_NO_DEADLINE", "1", 0);  // deterministic games: no wall-clock search limit
    if (argc != 6) {
        std::cerr << "usage: full_games opponent seed_start games threads out.csv\n";
        return 2;
    }
    const std::string opponent = argv[1];
    const bool bc = opponent.rfind("bc:", 0) == 0;
    if (bc) setenv("BC_OPPONENT_MODEL", opponent.substr(3).c_str(), 1);
    const bool dc11 = opponent.rfind("dc11:", 0) == 0;
#ifdef WITH_DC11
    if (dc11) setenv("BC_OPPONENT_MODEL", opponent.substr(5).c_str(), 1);
#else
    if (dc11) {
        std::cerr << "dc11: opponents need full_games_dc11\n";
        return 2;
    }
#endif
    if (!bc && !dc11 && opponent != "pass" && opponent != "agent_sep23") {
        setenv("BC_OPPONENT", argv[1], 1);
        if (!opponents::make(argv[1])) {
            std::cerr << "unknown opponent " << opponent << '\n';
            return 2;
        }
    }
    const uint64_t seed_start = std::strtoull(argv[2], nullptr, 10);
    const int games = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    // FULL_SEAT=<0|1>: one seat per seed (the maps are mirrored; both seats mostly repeat one game).
    const char* only_seat = std::getenv("FULL_SEAT");
    const int per_seed = only_seat ? 1 : 2;
    std::vector<Result> results(per_seed * games);
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int job; (job = next++) < per_seed * games;) {
                const uint64_t seed = seed_start + job / per_seed;
                const int seat = only_seat ? std::atoi(only_seat) : job % 2;
                results[job] = opponent == "pass"          ? play<Pass>(seed, seat)
                               : opponent == "agent_sep23" ? play<sep23::Opponent>(seed, seat)
                               : bc                        ? play<OtherBC>(seed, seat)
#ifdef WITH_DC11
                               : dc11                      ? play<OtherDC11>(seed, seat)
#endif
                                                           : play<Inhouse>(seed, seat);
            }
        });
    for (auto& t : pool) t.join();
    std::ofstream out(argv[5]);
    out << "seed,seat,own,rival,margin,uncompiled_days,fallback_days,invalid_intents,discarded,rival_discarded,"
           "compile_ms_max,land,rival_land,animals,rival_animals,escaped,rival_escaped,compile_ms_total,overage_left,overage_step,animals_d4,rival_animals_d4\n";
    int wins = 0;
    double margin = 0;
    for (const auto& r : results) {
        out << r.seed << ',' << r.seat << ',' << r.own << ',' << r.rival << ',' << r.own - r.rival << ',' << r.uncompiled
            << ',' << r.fallback << ',' << r.invalid << ',' << r.discarded << ',' << r.rival_discarded << ','
            << r.compile_max << ',' << r.land << ',' << r.rival_land << ',' << r.animals << ',' << r.rival_animals << ','
            << r.escaped << ',' << r.rival_escaped << ',' << r.compile_total << ',' << r.overage_left << ',' << r.overage_step << ',' << r.animals_d4 << ',' << r.rival_animals_d4 << '\n';
        wins += r.own > r.rival;
        margin += r.own - r.rival;
    }
    std::cout << opponent << " games " << results.size() << " wins " << wins << " mean margin " << margin / results.size() << '\n';
    return 0;
}
