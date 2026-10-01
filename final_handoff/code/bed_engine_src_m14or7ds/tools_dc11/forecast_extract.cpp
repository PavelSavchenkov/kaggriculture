// Opponent-sales forecast dataset. For each corpus line (episode seat team submission split trace) the trace is replayed
// from that seat; at every dawn we record what the agent can observe and dc11's own forecast, and after the game the
// opponent's realized net flow that day (dc10::History inference, the quantity the seller forecasts).
// Per dawn (float32): FEATURES scalars, then trailing 3-day flow, yesterday's flow and dc11's forecast (each 24 x 9),
// then the label (24 x 9). Meta (int64): episode, seat, day, split (0 train, 1 validation, 2 test).
// usage: forecast_extract corpus.txt shard shards out_prefix   -> out_prefix.x.f32, out_prefix.meta.i64
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "dc11/market.hpp"
#include "dc11_local/learned_forecast.hpp"
#include <fstream>
#include <memory>
#include <iostream>
#include <sstream>

using namespace dc10;

namespace {
using fcast::SCALARS;
constexpr int GRID = HOURS * N_PRODUCTS;
constexpr int WIDTH = SCALARS + 4 * GRID;  // + trailing, yesterday, dc11 forecast, label
}  // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: forecast_extract corpus.txt shard shards out_prefix\n";
        return 2;
    }
    const int shard = std::atoi(argv[2]), shards = std::atoi(argv[3]);
    std::ifstream corpus(argv[1]);
    std::ofstream xs(std::string(argv[4]) + ".x.f32", std::ios::binary), meta(std::string(argv[4]) + ".meta.i64", std::ios::binary);
    int line_no = 0, games = 0;
    dc11::MarketOptions options;  // v28 defaults: first_full 1, blend 0.5
    // FORECAST_MODEL: also the learned forecast per dawn (<out>.pred.f32), to check the C++ inference.
    std::unique_ptr<fcast::LearnedForecast> learned;
    std::ofstream preds;
    if (const char* path = std::getenv("FORECAST_MODEL")) {
        learned = std::make_unique<fcast::LearnedForecast>();
        if (!learned->load(path)) { std::cerr << "cannot load " << path << '\n'; return 2; }
        preds.open(std::string(argv[4]) + ".pred.f32", std::ios::binary);
    }
    for (std::string line; std::getline(corpus, line); ++line_no) {
        if (line_no % shards != shard) continue;
        std::istringstream in(line);
        int64_t episode = 0, team = 0, submission = 0;
        int seat = 0;
        std::string split, trace;
        if (!(in >> episode >> seat >> team >> submission >> split >> trace)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        History history;
        std::vector<std::vector<float>> rows;
        std::vector<int> days;
        for (size_t s = 0; s < replay.turns.size() && !sim.st.done; ++s) {
            const auto obs = kag::agent::runtime::make_observation(sim, seat);
            history.observe(obs, replay.turns[s][seat]);
            if (obs.hour == 0 && obs.day > 0 && obs.day < LAST_DAY) {
                std::vector<float> row(WIDTH, 0.0f);
                fcast::scalars(obs, history, row.data());
                double trail[HOURS][N_PRODUCTS], base[HOURS][N_PRODUCTS];
                history.expected(obs.day, 3, trail);
                dc11::forecast(obs, history, base, options.first_full, options.blend);
                if (learned) {
                    double pred[HOURS][N_PRODUCTS];
                    std::copy_n(&base[0][0], GRID, &pred[0][0]);
                    learned->apply(obs, history, pred);
                    for (int h = 0; h < HOURS; ++h)
                        for (int p = 0; p < N_PRODUCTS; ++p) { const float v = float(pred[h][p]); preds.write(reinterpret_cast<const char*>(&v), 4); }
                }
                for (int h = 0; h < HOURS; ++h)
                    for (int p = 0; p < N_PRODUCTS; ++p) {
                        row[SCALARS + h * N_PRODUCTS + p] = float(trail[h][p]);
                        row[SCALARS + GRID + h * N_PRODUCTS + p] = float(history.flow_at((obs.day - 1) * HOURS + h, p));
                        row[SCALARS + 2 * GRID + h * N_PRODUCTS + p] = float(base[h][p]);
                    }
                rows.push_back(std::move(row));
                days.push_back(obs.day);
            }
            sim.step(replay.turns[s][0], replay.turns[s][1]);
        }
        // The last step's flow is inferred at the next observation: one more observe to close the game.
        history.observe(kag::agent::runtime::make_observation(sim, seat), Action{});
        const int64_t split_id = split == "train" ? 0 : split == "validation" ? 1 : 2;
        for (size_t i = 0; i < rows.size(); ++i) {
            for (int h = 0; h < HOURS; ++h)
                for (int p = 0; p < N_PRODUCTS; ++p)
                    rows[i][SCALARS + 3 * GRID + h * N_PRODUCTS + p] = float(history.flow_at(days[i] * HOURS + h, p));
            xs.write(reinterpret_cast<const char*>(rows[i].data()), sizeof(float) * WIDTH);
            const int64_t m[4] = {episode, seat, days[i], split_id};
            meta.write(reinterpret_cast<const char*>(m), sizeof m);
        }
        if (++games % 200 == 0) std::cerr << "games " << games << '\n';
    }
    std::cerr << "done games " << games << '\n';
    return 0;
}
