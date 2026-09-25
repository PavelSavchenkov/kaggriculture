// Full games: bc_opus against an opponent (agent_sep23 or pass), both seats per seed.
// BC_PERTURB=p: on each of our dawns up to day 12, with probability p one decision push (as in
// tools/search_games) replaces today's decoding (on-policy value-model data with varied decisions).
// usage: full_games opponent seed_start games threads out.csv
#include "agent/bc_opus/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "opponents/sep23_adapter.hpp"
#include "opponents/opponent.hpp"
#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

using namespace dc10;

namespace {
struct Result {
    uint64_t seed = 0;
    int seat = 0;
    double own = 0, rival = 0;
    int uncompiled = 0, fallback = 0, invalid = 0, discarded = 0, rival_discarded = 0;
    double compile_max = 0;
    int land = 0, rival_land = 0, animals = 0, rival_animals = 0;
    int escaped = 0, rival_escaped = 0;  // animals lost (only escapes remove a placed animal)
};

int placed_animals(const Farm& farm) {
    int n = 0;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) n += farm.tiles[y][x].has_animal;
    return n;
}

template <class Opponent>
Result play(uint64_t seed, int seat) {
    Config config;
    config.seed = seed;
    Sim sim(config);
    kag::agents::bc_opus::Agent mine;
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
        mine.act(kag::agent::runtime::make_observation(sim, seat), budget, a);
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
        other.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, b);
        turns.push_back(seat == 0 ? std::array<Action, 2>{a, b} : std::array<Action, 2>{b, a});
        const int before = placed_animals(sim.st.farms[seat]), rival_before = placed_animals(sim.st.farms[1 - seat]);
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
        if (sim.st.day < 25) {  // before day 25 (later, unfed animals are often left to escape: no sale left)
            escaped += std::max(0, before - placed_animals(sim.st.farms[seat]));
            rival_escaped += std::max(0, rival_before - placed_animals(sim.st.farms[1 - seat]));
        }
    }
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
        r.invalid += !d.invalid.empty();
        r.compile_max = std::max(r.compile_max, d.compile_ms);
    }
    for (int i = 0; i < N_ITEMS; ++i) r.discarded += f.discarded[i], r.rival_discarded += g.discarded[i];
    r.land = f.n_quadrants, r.rival_land = g.n_quadrants;
    r.escaped = escaped, r.rival_escaped = rival_escaped;
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
    if (argc != 6) {
        std::cerr << "usage: full_games opponent seed_start games threads out.csv\n";
        return 2;
    }
    const std::string opponent = argv[1];
    const bool bc = opponent.rfind("bc:", 0) == 0;
    if (bc) setenv("BC_OPPONENT_MODEL", opponent.substr(3).c_str(), 1);
    if (!bc && opponent != "pass" && opponent != "agent_sep23") {
        setenv("BC_OPPONENT", argv[1], 1);
        if (!opponents::make(argv[1])) {
            std::cerr << "unknown opponent " << opponent << '\n';
            return 2;
        }
    }
    const uint64_t seed_start = std::strtoull(argv[2], nullptr, 10);
    const int games = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    std::vector<Result> results(2 * games);
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int job; (job = next++) < 2 * games;) {
                const uint64_t seed = seed_start + job / 2;
                const int seat = job % 2;
                results[job] = opponent == "pass"          ? play<Pass>(seed, seat)
                               : opponent == "agent_sep23" ? play<sep23::Opponent>(seed, seat)
                               : bc                        ? play<OtherBC>(seed, seat)
                                                           : play<Inhouse>(seed, seat);
            }
        });
    for (auto& t : pool) t.join();
    std::ofstream out(argv[5]);
    out << "seed,seat,own,rival,margin,uncompiled_days,fallback_days,invalid_intents,discarded,rival_discarded,"
           "compile_ms_max,land,rival_land,animals,rival_animals,escaped,rival_escaped\n";
    int wins = 0;
    double margin = 0;
    for (const auto& r : results) {
        out << r.seed << ',' << r.seat << ',' << r.own << ',' << r.rival << ',' << r.own - r.rival << ',' << r.uncompiled
            << ',' << r.fallback << ',' << r.invalid << ',' << r.discarded << ',' << r.rival_discarded << ','
            << r.compile_max << ',' << r.land << ',' << r.rival_land << ',' << r.animals << ',' << r.rival_animals << ','
            << r.escaped << ',' << r.rival_escaped << '\n';
        wins += r.own > r.rival;
        margin += r.own - r.rival;
    }
    std::cout << opponent << " games " << results.size() << " wins " << wins << " mean margin " << margin / results.size() << '\n';
    return 0;
}
