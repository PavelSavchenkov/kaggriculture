// Reactive loss-flip search: which one-day decode push would have turned a lost game into a win against a reactive opponent?
// Our dc11 agent (BC_OPUS_MODEL with its <model>.dc11 sidecar) plays seat 0 against a top-team clone (models/zoo_<team><FLIP_CLONE>,
// its own sidecar), SHOP_CRN as reports/nft/dc11bed.sh, so base games equal the clone-bed rows. Every fork is a fresh game replayed
// from the start with one push on its dawn (new-animal / new-crop quantile 0.8 or 0.2), both sides reacting afterwards. Forks are not
// copied from snapshots: the agent's learned-forecast state (TfState, exact History) sits behind shared pointers, so a copied game
// shares it with the main line and diverges. A "null" fork (push day set, no push values) must reproduce the base game.
// usage: flip_dc11 seed_start games threads out.csv   (env FLIP_TEAMS "azat decem dsm mm", FLIP_CLONE "17_fair", FLIP_FIRST 1,
//   FLIP_LAST 14, FLIP_MAX 0)  -> rows: seed, team, day, variant, margin (fork), base (main line)
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include "tools/shop_crn.hpp"

using namespace dc10;

namespace {
struct Variant {
    const char* name;
    double q_animal, q_crop;
};
const Variant VARIANTS[] = {{"A+", 0.8, -1}, {"A-", 0.2, -1}, {"C+", -1, 0.8}, {"C-", -1, 0.2}};

std::string sidecar(const std::string& model) {  // <model>.dc11 (as tools/full_games.cpp)
    std::string s;
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {  // step 67: the whole first line (no 255-character limit)
            s += chunk;
            if (s.back() == '\n') break;
        }
        std::fclose(file);
    }
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s.empty() ? "-" : s;
}

// One full game; push_day < 0: no push.
double play(uint64_t seed, const std::string& model, const std::string& opponent, int push_day, double q_animal, double q_crop) {
    Config config;
    config.seed = seed;
    Sim sim(config);
    int crn_shops = 0;
    kag::agents::bc_overhaul::Agent mine, other;
    mine.model_path = model;
    mine.options_text = sidecar(model);
    other.model_path = opponent;
    other.options_text = sidecar(opponent);
    mine.push_day = push_day, mine.push_q_animal = q_animal, mine.push_q_crop = q_crop;
    mine.reset(kag::agent::runtime::make_agent_init(sim, 0));
    other.reset(kag::agent::runtime::make_agent_init(sim, 1));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    while (!sim.st.done) {
        Action a, b;
        mine.act(kag::agent::runtime::make_observation(sim, 0), budget, a);
        other.act(kag::agent::runtime::make_observation(sim, 1), budget, b);
        sim.step(a, b);
        shop_crn(sim, crn_shops);
    }
    return sim.st.farms[0].money - sim.st.farms[1].money;
}

int env_int(const char* name, int fallback) {
    const char* v = std::getenv(name);
    return v && *v ? std::atoi(v) : fallback;
}

template <class F>
void parallel(int jobs, int threads, F f) {
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int job; (job = next++) < jobs;) f(job);
        });
    for (auto& t : pool) t.join();
}
}  // namespace

int main(int argc, char** argv) {
    setenv("SHOP_CRN", "1", 1);
    setenv("DC10_NO_DEADLINE", "1", 1);
    if (argc != 5) {
        std::cerr << "usage: flip_dc11 seed_start games threads out.csv\n";
        return 2;
    }
    const uint64_t seed_start = std::strtoull(argv[1], nullptr, 10);
    const int games = std::atoi(argv[2]), threads = std::atoi(argv[3]);
    std::vector<std::string> teams;
    {
        std::istringstream in(std::getenv("FLIP_TEAMS") ? std::getenv("FLIP_TEAMS") : "azat decem dsm mm");
        for (std::string t; in >> t;) teams.push_back(t);
    }
    const std::string clone = std::getenv("FLIP_CLONE") ? std::getenv("FLIP_CLONE") : "17_fair";
    const int first = env_int("FLIP_FIRST", 1), last = env_int("FLIP_LAST", 14), max_margin = env_int("FLIP_MAX", 0);
    const char* model = std::getenv("BC_OPUS_MODEL");
    if (!model) {
        std::cerr << "BC_OPUS_MODEL is required\n";
        return 2;
    }
    std::ofstream out(argv[4]);
    out << "seed,team,day,variant,margin,base\n";
    std::mutex lock;
    auto opponent = [&](const std::string& team) { return "models/zoo_" + team + clone + "/model.bin"; };
    // Phase 1: base games.
    const int base_jobs = games * int(teams.size());
    std::vector<double> base(base_jobs);
    parallel(base_jobs, threads, [&](int job) {
        const uint64_t seed = seed_start + job % games;
        const std::string& team = teams[job / games];
        base[job] = play(seed, model, opponent(team), -1, -1, -1);
        std::lock_guard<std::mutex> guard(lock);
        out << seed << ',' << team << ",-1,base," << base[job] << ',' << base[job] << '\n';
        out.flush();
    });
    // Phase 2: forks of the lost games (one null fork per game checks that replays are exact).
    struct Fork { int job, day, variant; };
    std::vector<Fork> forks;
    for (int job = 0; job < base_jobs; ++job)
        if (base[job] < max_margin) {
            forks.push_back({job, first, -1});
            for (int day = first; day <= last; ++day)
                for (int v = 0; v < 4; ++v) forks.push_back({job, day, v});
        }
    parallel(int(forks.size()), threads, [&](int k) {
        const Fork& f = forks[k];
        const uint64_t seed = seed_start + f.job % games;
        const std::string& team = teams[f.job / games];
        const bool null = f.variant < 0;
        const double margin = play(seed, model, opponent(team), f.day, null ? -1 : VARIANTS[f.variant].q_animal,
                                   null ? -1 : VARIANTS[f.variant].q_crop);
        std::lock_guard<std::mutex> guard(lock);
        out << seed << ',' << team << ',' << f.day << ',' << (null ? "null" : VARIANTS[f.variant].name) << ',' << margin << ','
            << base[f.job] << '\n';
        out.flush();
    });
    return 0;
}
