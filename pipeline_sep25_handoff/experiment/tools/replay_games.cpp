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

using namespace dc10;

namespace {
struct Game {
    std::string trace;
    int replayed = 0;
};

struct Result {
    uint64_t seed = 0;
    double own = 0, rival = 0;              // this game
    double original_rival = 0, original_other = 0;  // the recorded episode's final money
};

Result play(const Game& game) {
    const Replay replay = load_replay(game.trace);
    const auto original = replay_states(replay);
    const int seat = 1 - game.replayed;
    Sim sim(replay.config);
    kag::agents::bc_opus::Agent mine;
    mine.reset(kag::agent::runtime::make_agent_init(sim, seat));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 256;
    const bool trace = std::getenv("BC_REPLAY_TRACE") != nullptr;  // per dawn: farms and the compile report
    for (int step = 0; !sim.st.done; ++step) {
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
        seat == 0 ? sim.step(a, b) : sim.step(b, a);
    }
    Result r;
    r.seed = replay.config.seed;
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
    out << "trace,replayed_seat,seed,own,rival,margin,original_rival,original_other\n";
    int wins = 0;
    double margin = 0;
    for (size_t i = 0; i < games.size(); ++i) {
        const auto& r = results[i];
        out << games[i].trace << ',' << games[i].replayed << ',' << r.seed << ',' << r.own << ',' << r.rival << ','
            << r.own - r.rival << ',' << r.original_rival << ',' << r.original_other << '\n';
        wins += r.own > r.rival;
        margin += r.own - r.rival;
    }
    std::cout << "games " << games.size() << " wins " << wins << " mean margin " << margin / games.size() << '\n';
    return 0;
}
