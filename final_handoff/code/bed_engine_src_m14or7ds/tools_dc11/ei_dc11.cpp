// Counterfactual one-dawn decode pushes under dc11 (expert-iteration headroom probe). Our dc11 agent (BC_OPUS_MODEL)
// plays seat 0 against a top-team clone run by dc11 (team by seed from EI_TEAMS), shop CRN always on. On a few
// sampled dawns the game is forked: each variant pushes that dawn's decode only (new-animal or new-crop quantile,
// land logit) and the fork plays to the end. Rows: seed, team, day, variant, margin (fork), base (main line), and
// the fork's and main line's new animals / crops decoded that dawn.
// usage: ei_dc11 seed_start games threads out.csv
//   env: EI_TEAMS (default the six original clones), EI_DAYS_PER_GAME (3), EI_FIRST (1), EI_LAST (20)
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include "tools/shop_crn.hpp"

using namespace dc10;

namespace {
struct Variant {
    const char* name;
    double q_animal, q_crop, land;
};
const Variant VARIANTS[] = {{"qa25", 0.25, -1, 0}, {"qa80", 0.8, -1, 0}, {"qc25", -1, 0.25, 0},
                            {"qc80", -1, 0.8, 0},  {"land+3", -1, -1, 3}, {"land-3", -1, -1, -3}};

struct Game {
    Sim sim;
    kag::agents::bc_overhaul::Agent mine, other;
    int crn_shops = 0;
    void step() {
        kag::agent::DecisionBudget budget;
        budget.max_expansions = 256;
        Action a, b;
        mine.act(kag::agent::runtime::make_observation(sim, 0), budget, a);
        other.act(kag::agent::runtime::make_observation(sim, 1), budget, b);
        sim.step(a, b);
        shop_crn(sim, crn_shops);
    }
    double margin() const { return sim.st.farms[0].money - sim.st.farms[1].money; }
};

double finish(Game g) {  // a copy: the fork
    while (!g.sim.st.done) g.step();
    return g.margin();
}

int env_int(const char* name, int fallback) {
    const char* v = std::getenv(name);
    return v && *v ? std::atoi(v) : fallback;
}
}  // namespace

int main(int argc, char** argv) {
    setenv("SHOP_CRN", "1", 1);
    if (argc != 5) {
        std::cerr << "usage: ei_dc11 seed_start games threads out.csv\n";
        return 2;
    }
    const uint64_t seed_start = std::strtoull(argv[1], nullptr, 10);
    const int games = std::atoi(argv[2]), threads = std::atoi(argv[3]);
    std::vector<std::string> teams;
    {
        std::istringstream in(std::getenv("EI_TEAMS") ? std::getenv("EI_TEAMS") : "dsm decem goose majkel mm vadim");
        for (std::string t; in >> t;) teams.push_back(t);
    }
    const int per_game = env_int("EI_DAYS_PER_GAME", 3), first = env_int("EI_FIRST", 1), last = env_int("EI_LAST", 20);
    const char* model = std::getenv("BC_OPUS_MODEL");
    if (!model) {
        std::cerr << "BC_OPUS_MODEL is required\n";
        return 2;
    }
    std::ofstream out(argv[4]);
    out << "seed,team,day,variant,margin,base\n";
    std::mutex lock;
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int job; (job = next++) < games;) {
                const uint64_t seed = seed_start + job;
                const std::string team = teams[seed % teams.size()];
                Config config;
                config.seed = seed;
                Game g{Sim(config), {}, {}, 0};
                g.mine.model_path = model;
                g.other.model_path = "models/zoo_" + team + "_race/model.bin";
                g.other.options_text = "-";
                g.mine.reset(kag::agent::runtime::make_agent_init(g.sim, 0));
                g.other.reset(kag::agent::runtime::make_agent_init(g.sim, 1));
                std::vector<int> days;
                for (int d = first; d <= last; ++d) days.push_back(d);
                std::mt19937_64 rng(seed);
                std::shuffle(days.begin(), days.end(), rng);
                days.resize(std::min<int>(per_game, int(days.size())));
                std::vector<std::tuple<int, std::string, double>> forks;
                while (!g.sim.st.done) {
                    if (g.sim.st.step % HOURS == 0 && std::find(days.begin(), days.end(), g.sim.st.day) != days.end())
                        for (const auto& v : VARIANTS) {
                            Game f = g;
                            f.mine.push_day = g.sim.st.day;
                            f.mine.push_q_animal = v.q_animal, f.mine.push_q_crop = v.q_crop, f.mine.push_land = v.land;
                            forks.emplace_back(g.sim.st.day, v.name, finish(f));
                        }
                    g.step();
                }
                const double base = g.margin();
                std::lock_guard<std::mutex> guard(lock);
                for (const auto& [day, name, margin] : forks)
                    out << seed << ',' << team << ',' << day << ',' << name << ',' << margin << ',' << base << '\n';
                out.flush();
            }
        });
    for (auto& t : pool) t.join();
    return 0;
}
