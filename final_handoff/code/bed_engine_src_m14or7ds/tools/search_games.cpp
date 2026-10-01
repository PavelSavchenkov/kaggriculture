// Full games with optional per-dawn search, against a cloneable opponent: a registered
// in-house agent (opponents/) or another bc_opus model ("bc:<model.bin>").
// Search: at each of our dawns up to SEARCH_LAST_DAY (default 20), every candidate
// decision push is played out with copies of both agents and the engine to the end of the
// game (baseline pushes after that day) and the candidate with the best final margin is
// used. The baseline is a candidate, so with an exact opponent copy the searched game never
// ends worse than the unsearched one: it measures how much better decisions alone gain.
// SEARCH_SPACE=compiler: the candidates are compiler-option variants instead of decision pushes
// (same count), to compare where the per-dawn headroom sits.
// SEARCH_ROLLOUTS=<dir>: every rollout is saved as a trace <dir>/<seed>_<seat>_<day>_<candidate>.txt
// and its final margin appended to <dir>/rollouts.csv (value-model decision test).
// usage: search_games opponent seed_start games threads out.csv [search 0|1]
#include "agent/bc_opus/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "opponents/opponent.hpp"
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>
#include "tools/shop_crn.hpp"

using namespace dc10;
using kag::agents::bc_opus::Agent;
using kag::agents::bc_opus::DecodeKnobs;

namespace {
// An opponent that can be copied for rollouts.
struct Rival {
    std::unique_ptr<opponents::Opponent> inhouse;
    std::unique_ptr<Agent> bc;
    static Rival make(const std::string& name) {
        Rival r;
        if (name.rfind("bc:", 0) == 0) {
            r.bc = std::make_unique<Agent>();
            r.bc->knobs = DecodeKnobs{};
            r.bc->model_path = name.substr(3);
            r.bc->opening_days = kag::agents::bc_opus::opening_experiment() ? 0 : -1;  // BC_OPENING_* is for the agent under test
        } else {
            r.inhouse = opponents::make(name.c_str());
        }
        return r;
    }
    Rival copy() const {
        Rival r;
        if (bc) r.bc = std::make_unique<Agent>(*bc);
        if (inhouse) r.inhouse = inhouse->clone();
        return r;
    }
    bool valid() const { return bc || inhouse; }
    void reset(const kag::agent::AgentInit& init) { bc ? bc->reset(init) : inhouse->reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, Action& a) {
        bc ? bc->act(o, b, a) : inhouse->act(o, b, a);
    }
};

struct Candidate {
    DecodeKnobs knobs;
    CompileOptions options;
};

std::vector<Candidate> candidates(const CompileOptions& base) {
    std::vector<Candidate> out{{DecodeKnobs{}, base}};  // baseline first
    auto push = [&](auto change) {
        Candidate c{DecodeKnobs{}, base};
        change(c);
        out.push_back(c);
    };
    if (const char* space = std::getenv("SEARCH_SPACE"); space && std::string(space) == "compiler") {
        push([](Candidate& c) { c.options.hold_discount = 0.85; });
        push([](Candidate& c) { c.options.hold_discount = 1.0; });
        push([](Candidate& c) { c.options.cash_margin = 300; });
        push([](Candidate& c) { c.options.collect_fertilizer = false; });
        push([](Candidate& c) { c.options.return_output = false; });
        push([](Candidate& c) { c.options.water_for_production = false; });
        return out;
    }
    push([](Candidate& c) { c.knobs.q_crop = 0.3; });
    push([](Candidate& c) { c.knobs.q_crop = 0.7; });
    push([](Candidate& c) { c.knobs.q_animal = 0.3; });
    push([](Candidate& c) { c.knobs.q_animal = 0.7; });
    push([](Candidate& c) { c.knobs.land_bias = 3; });
    push([](Candidate& c) { c.knobs.land_bias = -3; });
    return out;
}

struct Result {
    uint64_t seed = 0;
    int seat = 0;
    double own = 0, rival = 0;
    int searched = 0, changed = 0;
    std::string choices;  // per searched day: index of the chosen candidate
    int escaped = 0, rival_escaped = 0;  // animals lost (only escapes remove a placed animal)
};

int placed_animals(const Farm& farm) {
    int n = 0;
    for (int y = 0; y < BOARD; ++y)
        for (int x = 0; x < BOARD; ++x) n += farm.tiles[y][x].has_animal;
    return n;
}

// Plays from the current state to the end with `today`'s candidate for `mine` (today only).
double rollout(Sim sim, Agent mine, Rival rival, int seat, const Candidate& today, const CompileOptions& base,
               std::vector<std::array<Action, 2>>* turns) {
    int crn_shops = sim.st.n_shops;
    kag::agent::DecisionBudget budget;
    const int day = sim.st.day;
    mine.knobs = today.knobs;
    mine.compile_options() = today.options;
    while (!sim.st.done) {
        if (sim.st.day != day) mine.knobs = DecodeKnobs{}, mine.compile_options() = base;
        Action a, b;
        mine.act(kag::agent::runtime::make_observation(sim, seat), budget, a);
        rival.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, b);
        if (turns) turns->push_back(seat == 0 ? std::array<Action, 2>{a, b} : std::array<Action, 2>{b, a});
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
        shop_crn(sim, crn_shops);
    }
    return sim.st.farms[seat].money - sim.st.farms[1 - seat].money;
}

Result play(const std::string& opponent, uint64_t seed, int seat, bool search, int last_day, const float* oracle = nullptr,
            std::vector<float>* record = nullptr) {
    Config config;
    config.seed = seed;
    Sim sim(config);
    int crn_shops = 0;  // SHOP_CRN: shops fixed so far
    Agent mine;
    mine.knobs = DecodeKnobs{};
    mine.reset(kag::agent::runtime::make_agent_init(sim, seat));
    if (oracle) mine.compile_options().oracle_rival = oracle;
    Rival rival = Rival::make(opponent);
    rival.reset(kag::agent::runtime::make_agent_init(sim, 1 - seat));
    // SEARCH_MODEL=<opponent>: rollouts use copies of this shadow agent, which sees the real
    // game's observations (its actions are ignored), instead of the true opponent.
    const char* model_name = std::getenv("SEARCH_MODEL");
    Rival model = model_name ? Rival::make(model_name) : Rival{};
    if (model_name) model.reset(kag::agent::runtime::make_agent_init(sim, 1 - seat));
    kag::agent::DecisionBudget budget;
    const CompileOptions base = mine.compile_options();
    const auto options = candidates(base);
    std::vector<std::array<Action, 2>> turns;  // SEARCH_TRACES=<dir>: the game as a trace
    Result r;
    while (!sim.st.done) {
        mine.knobs = DecodeKnobs{};
        if (sim.st.step % HOURS == 1) mine.compile_options() = base;  // today's plan and executor keep their copy
        if (search && sim.st.step % HOURS == 0 && sim.st.day <= last_day) {
            int best = 0;
            double best_margin = -1e18;
            const char* rollouts = std::getenv("SEARCH_ROLLOUTS");
            for (size_t c = 0; c < options.size(); ++c) {
                auto played = turns;
                const double margin = rollout(sim, mine, model_name ? model.copy() : rival.copy(), seat, options[c], base,
                                              rollouts ? &played : nullptr);
                if (rollouts) {
                    const std::string name = std::to_string(seed) + "_" + std::to_string(seat) + "_" +
                                             std::to_string(sim.st.day) + "_" + std::to_string(c);
                    save_replay(std::string(rollouts) + "/" + name + ".txt", config, played);
                    static std::mutex lock;
                    std::lock_guard guard(lock);
                    std::ofstream(std::string(rollouts) + "/rollouts.csv", std::ios::app)
                        << seed << ',' << seat << ',' << sim.st.day << ',' << c << ',' << margin << '\n';
                }
                if (margin > best_margin) best_margin = margin, best = int(c);
            }
            mine.knobs = options[best].knobs;
            mine.compile_options() = options[best].options;
            ++r.searched;
            r.changed += best != 0;
            r.choices += std::to_string(best);
        }
        Action a, b;
        mine.act(kag::agent::runtime::make_observation(sim, seat), budget, a);
        rival.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, b);
        if (model_name) {
            Action ignored;
            model.act(kag::agent::runtime::make_observation(sim, 1 - seat), budget, ignored);
        }
        turns.push_back(seat == 0 ? std::array<Action, 2>{a, b} : std::array<Action, 2>{b, a});
        const int before = placed_animals(sim.st.farms[seat]), rival_before = placed_animals(sim.st.farms[1 - seat]);
        int32_t rival_sold[N_PRODUCTS];
        std::copy_n(sim.st.farms[1 - seat].sold_units, N_PRODUCTS, rival_sold);
        const int step = sim.st.step;
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
        shop_crn(sim, crn_shops);
        if (record && step < 30 * HOURS)
            for (int p = 0; p < N_PRODUCTS; ++p) (*record)[step * N_PRODUCTS + p] = float(sim.st.farms[1 - seat].sold_units[p] - rival_sold[p]);
        if (sim.st.day < 25) {  // before day 25 (later, unfed animals are often left to escape: no sale left)
            r.escaped += std::max(0, before - placed_animals(sim.st.farms[seat]));
            r.rival_escaped += std::max(0, rival_before - placed_animals(sim.st.farms[1 - seat]));
        }
    }
    if (const char* dir = std::getenv("SEARCH_TRACES"))
        save_replay(std::string(dir) + "/" + std::to_string(seed) + "_" + std::to_string(seat) + ".txt", config, turns);
    r.seed = seed;
    r.seat = seat;
    r.own = sim.st.farms[seat].money;
    r.rival = sim.st.farms[1 - seat].money;
    return r;
}
}

int main(int argc, char** argv) {
    setenv("DC10_NO_DEADLINE", "1", 0);  // deterministic games: no wall-clock search limit
    if (argc < 6) {
        std::cerr << "usage: search_games opponent seed_start games threads out.csv [search 0|1]\n";
        return 2;
    }
    const std::string opponent = argv[1];
    Rival probe = Rival::make(opponent);
    if (!probe.valid() || !probe.copy().valid()) {
        std::cerr << "opponent " << opponent << " is unknown or cannot be copied\n";
        return 2;
    }
    const uint64_t seed_start = std::strtoull(argv[2], nullptr, 10);
    const int games = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    const bool search = argc > 6 && std::atoi(argv[6]);
    const char* last = std::getenv("SEARCH_LAST_DAY");
    const int last_day = last ? std::atoi(last) : 20;
    // SEARCH_SEAT=<0|1>: one seat per seed only (the maps are mirrored, so the other seat of a
    // self-play or same-agent game repeats the same decisions).
    const char* only_seat = std::getenv("SEARCH_SEAT");
    const int per_seed = only_seat ? 1 : 2;
    std::vector<Result> results(per_seed * games);
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int job; (job = next++) < per_seed * games;)
            {
                const uint64_t seed = seed_start + job / per_seed;
                const int seat = only_seat ? std::atoi(only_seat) : job % 2;
                // SEARCH_ORACLE=1 (value probe): play once recording the opponent's hourly sales, then
                // again with them as our agent's forecast (the opponent reacts, so it is approximate).
                if (std::getenv("SEARCH_ORACLE")) {
                    std::vector<float> table(30 * HOURS * N_PRODUCTS, 0.0f);
                    play(opponent, seed, seat, false, last_day, nullptr, &table);
                    results[job] = play(opponent, seed, seat, search, last_day, table.data());
                } else {
                    results[job] = play(opponent, seed, seat, search, last_day);
                }
            }
        });
    for (auto& t : pool) t.join();
    std::ofstream out(argv[5]);
    out << "seed,seat,own,rival,margin,searched_days,changed_days,choices,escaped,rival_escaped\n";
    int wins = 0;
    double margin = 0;
    for (const auto& r : results) {
        out << r.seed << ',' << r.seat << ',' << r.own << ',' << r.rival << ',' << r.own - r.rival << ',' << r.searched << ','
            << r.changed << ',' << r.choices << ',' << r.escaped << ',' << r.rival_escaped << '\n';
        wins += r.own > r.rival;
        margin += r.own - r.rival;
    }
    std::cout << opponent << (search ? " search" : "") << " games " << results.size() << " wins " << wins
              << " mean margin " << margin / results.size() << '\n';
    return 0;
}
