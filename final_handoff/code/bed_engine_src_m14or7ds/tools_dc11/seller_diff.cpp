// Seller diff (local, Imitation; DEVIATIONS.md I2 / I4): on every hour of a recorded player's days, our dc11 seller (BC_OPUS_MODEL's
// .dc11 keys) chooses this hour's lots from the recorded state (its shed, the book, the recording's deposits for the rest of the day as
// incoming, the night room the recording left; as duel_mm's DUEL_REC path) and is compared with what the recorded player sold that
// hour. Teacher-forced: the state is always the recording's, so every hour is an independent decision. One row per game-hour-product
// where either side sells or holds stock: trace,day,hour,product,ours,rec,shed,pockets,inventory,opp_sold,opp_shed
// SELLDIFF_PRODUCTS="3 5 6 7" (default: strawberry, egg, milk, wool). SELLDIFF_MODEL=<BC's I1 sellmodel .txt>: column `model` = the
// learned M&M seller's sampled units at the same hour (same cap; SELLDIFF_THRESHOLD=1: its threshold decode), fed its own History of the recording and the recording's sold today.
// usage: seller_diff list.txt first_day last_day threads out.csv   (list line: trace seat; seat = the recorded player)
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "source/convert.hpp"
#include "dc11_local/sellmodel.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

using namespace dc10;

namespace {
std::string sidecar(const std::string& model) {
    char text[4096] = {0};
    if (std::FILE* f = std::fopen((model + ".dc11").c_str(), "r")) {
        if (!std::fgets(text, sizeof text, f)) text[0] = 0;
        std::fclose(f);
    }
    std::string s = text;
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: seller_diff list.txt first_day last_day threads out.csv\n";
        return 2;
    }
    setenv("DC10_NO_DEADLINE", "1", 0);
    std::vector<std::pair<std::string, int>> games;
    std::ifstream list(argv[1]);
    for (std::string line; std::getline(list, line);) {
        std::istringstream w(line);
        std::string t;
        int s;
        if (w >> t >> s) games.push_back({t, s});
    }
    const int first = std::atoi(argv[2]), last = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    const char* model = std::getenv("BC_OPUS_MODEL");
    if (!model) {
        std::cerr << "BC_OPUS_MODEL is required\n";
        return 2;
    }
    bool dp[N_PRODUCTS]{};
    {
        std::string text = std::getenv("SELLDIFF_PRODUCTS") ? std::getenv("SELLDIFF_PRODUCTS") : "3 5 6 7";
        std::replace(text.begin(), text.end(), ',', ' ');
        std::istringstream in(text);
        for (int p; in >> p;) dp[p] = true;
    }
    std::ofstream out(argv[5]);
    bcsell::Model sm;
    const char* sm_path = std::getenv("SELLDIFF_MODEL");
    const bool threshold = std::getenv("SELLDIFF_THRESHOLD") != nullptr;  // BC's threshold decode (deterministic) instead of sampling
    if (sm_path && !sm.load(sm_path)) {
        std::cerr << "cannot load " << sm_path << "\n";
        return 2;
    }
    out << "trace,day,hour,product,ours,rec,shed,pockets,inventory,opp_sold,opp_shed,model\n";
    const std::string opts = sidecar(model);
    std::mutex lock;
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int g; (g = next++) < int(games.size());) {
                const auto& [trace, seat] = games[g];
                const Replay replay = load_replay(trace);
                const auto states = replay_states(replay);
                kag::agents::bc_overhaul::Agent agent;
                agent.model_path = model;
                agent.options_text = !opts.empty() ? opts : "-";
                agent.reset(kag::agent::runtime::make_agent_init(states[0], seat));
                std::ostringstream rows;
                dc11::DayMarket market;
                int incoming[HOURS][N_PRODUCTS]{}, room = 0;
                dc10::History hist;  // the learned seller's view of the recording (as the agent's history)
                for (int step = 0; step + 1 < int(states.size()) && step / HOURS <= last; ++step) {
                    const int day = step / HOURS, hour = step % HOURS;
                    const auto obs = kag::agent::runtime::make_observation(states[step], seat);
                    agent.seller_advance(obs);
                    hist.advance(obs);
                    if (day >= first && (day + 1) * HOURS + 1 < int(states.size())) {
                        if (hour == 0) {
                            market = agent.seller_market(obs);
                            // the recording's deposits per hour (cumulative) and the night room it left, as duel_mm's DUEL_REC path
                            for (int p = 0; p < N_PRODUCTS; ++p) {
                                int cum = 0;
                                for (int h = 0; h < HOURS; ++h) {
                                    const int j = day * HOURS + h;
                                    const Farm &f0 = states[j].st.farms[seat], &f1 = states[j + 1].st.farms[seat];
                                    const int sold = f1.sold_units[p] - f0.sold_units[p];
                                    cum += h == HOURS - 1 ? std::max(0, sold - int(f0.shed[p]))
                                                          : std::max(0, int(f1.shed[p]) - int(f0.shed[p]) + sold);
                                    incoming[h][p] = dp[p] ? cum : 0;
                                }
                            }
                            const Farm &nx = states[(day + 1) * HOURS].st.farms[seat], &h23 = states[(day + 1) * HOURS - 1].st.farms[seat];
                            int other = 0, pockets = 0;
                            for (int i = 0; i < N_ITEMS; ++i)
                                if (i >= N_PRODUCTS || !dp[i]) other += nx.shed[i];
                            for (int p = 0; p < N_PRODUCTS; ++p)
                                if (dp[p]) pockets += std::max(0, int(nx.shed[p]) - std::max(0, int(h23.shed[p]) - (nx.sold_units[p] - h23.sold_units[p])));
                            room = std::max(0, 100 - other - pockets - 2);
                        }
                        dc11::DayMarket m = market;
                        agent.seller_hour(obs, m);
                        int reserve[N_PRODUCTS]{}, sell[N_PRODUCTS]{};
                        for (int i = 0; i < N_PRODUCTS; ++i) reserve[i] = dp[i] ? 0 : 1 << 20;
                        dc11::choose_sales(obs, incoming, HOURS, m, reserve, room, sell);
                        const Farm &f0 = states[step].st.farms[seat], &f1 = states[step + 1].st.farms[seat];
                        const Farm &o0 = states[step].st.farms[1 - seat], &o1 = states[step + 1].st.farms[1 - seat];
                        int learned[N_PRODUCTS]{}, sold_today[N_PRODUCTS]{};
                        if (sm_path) {
                            const Farm& dawn = states[day * HOURS].st.farms[seat];
                            for (int p = 0; p < N_PRODUCTS; ++p) sold_today[p] = f0.sold_units[p] - dawn.sold_units[p];
                            bcsell::sell_units(sm, obs, hist, sold_today, std::hash<std::string>{}(trace), learned, threshold);
                        }
                        for (int p = 0; p < N_PRODUCTS; ++p) {
                            if (!dp[p]) continue;
                            int pk = 0;
                            for (int u = 0; u < f0.n_units; ++u) pk += f0.inv[u][p];
                            // worker actions run before market orders: this hour's deposits are sellable
                            const int deposit = incoming[hour][p] - (hour ? incoming[hour - 1][p] : 0);
                            const int ours = std::min<int>(sell[p], f0.shed[p] + deposit), rec = f1.sold_units[p] - f0.sold_units[p];
                            if (!ours && !rec && !f0.shed[p] && !deposit && !learned[p]) continue;
                            rows << trace << ',' << day << ',' << hour << ',' << p << ',' << ours << ',' << rec << ',' << int(f0.shed[p]) << ','
                                 << pk << ',' << states[step].st.market.inventory[p] << ',' << (o1.sold_units[p] - o0.sold_units[p]) << ','
                                 << int(o0.shed[p]) << ',' << std::min<int>(learned[p], f0.shed[p] + deposit) << '\n';
                        }
                    }
                    agent.observe(obs, replay.turns[step][seat]);  // history + the learned forecaster's dawns, in order
                    hist.observe(obs, replay.turns[step][seat]);
                }
                std::lock_guard<std::mutex> guard(lock);
                out << rows.str();
            }
        });
    for (auto& t : pool) t.join();
}
