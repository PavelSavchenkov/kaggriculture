#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include "agents/common/runtime/observation_builder.hpp"
#include "profile.hpp"

namespace bohann_catalog_tests {
struct Pass {
    static kag::agent::AgentInfo info() { return {"pass"}; }
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget&, kag::Action& a) {
        a.clear(); a.n_units = o.self().n_units;
        std::fill_n(a.units, a.n_units, kag::UnitAction{}); a.finalize();
    }
};

inline uint64_t random_word(uint64_t& state) {
    uint64_t v = (state += 0x9e3779b97f4a7c15ULL);
    v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9ULL;
    v = (v ^ (v >> 27)) * 0x94d049bb133111ebULL;
    return v ^ (v >> 31);
}

struct Options {
    std::string a = "teammate_shoprouter", b = "pass", output;
    std::vector<uint64_t> seeds;
    uint64_t seed_start = 1000, expansions = 100000;
    int games = 8, threads = 4, seat_mode = 2;
    bool validate = false, native_shops = false, profile = false;
};

inline Options options(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--validate") { o.validate = true; continue; }
        if (key == "--native-shops") { o.native_shops = true; continue; }
        if (key == "--profile") { o.profile = true; continue; }
        if (++i >= argc) std::abort();
        const std::string value = argv[i];
        if (key == "--a") o.a = value;
        else if (key == "--b") o.b = value;
        else if (key == "--output") o.output = value;
        else if (key == "--games") o.games = std::stoi(value);
        else if (key == "--threads") o.threads = std::stoi(value);
        else if (key == "--seed-start") o.seed_start = std::stoull(value);
        else if (key == "--budget-expansions") o.expansions = std::stoull(value);
        else if (key == "--seat-mode") o.seat_mode = value == "both" ? 2 : std::stoi(value);
        else if (key == "--seed-file") {
            std::ifstream file(value); if (!file) std::abort();
            uint64_t seed; while (file >> seed) o.seeds.push_back(seed);
            if (!file.eof() || o.seeds.empty()) std::abort();
        } else { std::fprintf(stderr, "unknown option %s\n", key.c_str()); std::abort(); }
    }
    if (o.games <= 0 || o.threads <= 0 || o.seat_mode < 0 || o.seat_mode > 2) std::abort();
    if (o.seeds.empty()) for (int i = 0; i < o.games; ++i) o.seeds.push_back(o.seed_start + i);
    return o;
}

struct Outcome {
    uint64_t seed = 0, hash[2] = {14695981039346656037ULL, 14695981039346656037ULL};
    int seat = 0, turns = 0;
    double cash[2]{}, revenue[2]{}, spend[2]{};
    int produced[2][kag::N_ITEMS]{}, sold[2][kag::N_ITEMS]{}, discarded[2][kag::N_ITEMS]{};
    int faults[2]{}, actions[2]{}, worker_days[2]{}, max_workers[2]{};
    std::array<uint8_t, 8> shops{};
    std::unique_ptr<DetailedProfile> profile;
};

inline void hash_action(uint64_t& hash, const kag::Action& a) {
    auto add = [&](int v) { hash ^= static_cast<uint32_t>(v); hash *= 1099511628211ULL; };
    add(a.n_units); add(a.n_orders);
    for (int i = 0; i < a.n_units; ++i) { add(a.units[i].op); add(a.units[i].arg); add(a.units[i].n); }
    for (int i = 0; i < a.n_orders; ++i) { add(a.orders[i].op); add(a.orders[i].item); add(a.orders[i].n); }
}

inline void validate_action(const kag::Action& a, const kag::agent::AgentObservation& o) {
    if (a.n_units != o.self().n_units || a.n_orders < 0 || a.n_orders > 10 || !a.metadata_ready) std::abort();
    auto expected = a; expected.finalize();
    if (a.plant_mask != expected.plant_mask) std::abort();
    for (int c = 0; c < kag::N_CROPS; ++c)
        if (a.plant_demand[c] != expected.plant_demand[c]) std::abort();
    for (int u = 0; u < a.n_units; ++u)
        if (a.units[u].op >= kag::OP_INVALID) std::abort();
}

template<class AgentA, class AgentB>
Outcome run_game(AgentA& a, AgentB& b, uint64_t seed, int seat, const Options& o) {
    kag::Config config; config.seed = seed;
    kag::Sim sim(config);
    a.reset(kag::agent::runtime::make_agent_init(sim, seat));
    b.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
    kag::agent::DecisionBudget budget; budget.max_expansions = o.expansions;
    Outcome result; result.seed = seed; result.seat = seat;
    if(o.profile) result.profile=std::make_unique<DetailedProfile>();
    uint64_t shop_rng = seed ^ 0xa37108e62d045fb9ULL;
    for (auto& shop : result.shops) shop = random_word(shop_rng) % kag::N_SHOPS;
    while (!sim.st.done) {
        // Sample shops independently of policy-dependent weed RNG consumption.
        // They are exposed only when the exact engine unlocks each shop.
        if (!o.native_shops) std::copy_n(result.shops.begin(), sim.st.n_shops, sim.st.shops);
        const auto oa = kag::agent::runtime::make_observation(sim, seat);
        const auto ob = kag::agent::runtime::make_observation(sim, seat ^ 1);
        kag::Action acts[2];
        a.act(oa, budget, acts[seat]); b.act(ob, budget, acts[seat ^ 1]);
        validate_action(acts[seat], oa); validate_action(acts[seat ^ 1], ob);
        for (int p = 0; p < 2; ++p) {
            hash_action(result.hash[p], acts[p]);
            result.max_workers[p] = std::max(result.max_workers[p], sim.st.farms[p].n_units);
            if (sim.st.hour == 22 || sim.st.hour == 23) {
                if (sim.st.hour == 23 || sim.st.day == 29) result.worker_days[p] += sim.st.farms[p].n_units;
            }
        }
        if (o.validate) {
            const auto d = sim.diagnose_joint_actions(acts[0], acts[1]);
            for (int p = 0; p < 2; ++p) {
                result.actions[p] += d.players[p].requested_unit_actions;
                result.faults[p] += d.players[p].requested_unit_actions - d.players[p].successful_unit_actions;
            }
        }
        if(o.profile) {
            const auto before=sim;
            sim.step(acts[0],acts[1]);
            result.profile->observe(before,sim,acts);
        } else sim.step(acts[0], acts[1]);
    }
    result.turns = sim.st.step;
    if (result.turns != 719 || sim.st.n_shops != 8) std::abort();
    if (o.native_shops) std::copy_n(sim.st.shops, 8, result.shops.begin());
    for (int p = 0; p < 2; ++p) {
        const auto& f = sim.st.farms[p];
        result.cash[p] = f.money; result.revenue[p] = f.sell_revenue; result.spend[p] = f.total_spend;
        std::copy_n(f.produced, kag::N_ITEMS, result.produced[p]);
        std::copy_n(f.sold_units, kag::N_ITEMS, result.sold[p]);
        std::copy_n(f.discarded, kag::N_ITEMS, result.discarded[p]);
    }
    return result;
}

inline void write_results(const Options& o, const std::vector<Outcome>& results, double seconds) {
    std::ofstream file(o.output); if (!file) std::abort();
    file << "{\"agent_a\":\"" << o.a << "\",\"agent_b\":\"" << o.b << "\",\"seconds\":" << seconds
         << ",\"scenario\":\"" << (o.native_shops ? "official_native_rng" : "official_rules_independent_shop_stream")
         << "\",\"validated\":" << (o.validate ? "true" : "false") << ",\"games\":[";
    double utility = 0, cash = 0, margin = 0;
    std::vector<double> tails, margins;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i]; const int p = r.seat, q = p ^ 1;
        if (i) file << ',';
        file << "{\"seed\":" << r.seed << ",\"seat\":" << p << ",\"cash\":" << r.cash[p]
             << ",\"opponent_cash\":" << r.cash[q] << ",\"turns\":" << r.turns
             << ",\"action_hash\":\"" << r.hash[p] << "\",\"opponent_action_hash\":\"" << r.hash[q]
             << "\",\"unit_faults\":" << r.faults[p] << ",\"opponent_unit_faults\":" << r.faults[q]
             << ",\"worker_days\":" << r.worker_days[p] << ",\"max_workers\":" << r.max_workers[p]
             << ",\"revenue\":" << r.revenue[p] << ",\"spend\":" << r.spend[p];
        auto array = [&](const char* key, const auto& values, int n) {
            file << ",\"" << key << "\":[";
            for (int j = 0; j < n; ++j) { if (j) file << ','; file << +values[j]; }
            file << ']';
        };
        array("produced", r.produced[p], kag::N_ITEMS); array("sold", r.sold[p], kag::N_ITEMS);
        array("discarded", r.discarded[p], kag::N_ITEMS); array("shops", r.shops, 8);
        if(r.profile) {
            file << ",\"profile\":"; r.profile->write(file,p);
            file << ",\"opponent_profile\":"; r.profile->write(file,q);
        }
        file << '}';
        utility += r.cash[p] > r.cash[q] ? 1.0 : r.cash[p] == r.cash[q] ? 0.5 : 0.0;
        cash += r.cash[p]; margin += r.cash[p] - r.cash[q];
        tails.push_back(r.cash[p]); margins.push_back(r.cash[p] - r.cash[q]);
    }
    std::sort(tails.begin(), tails.end()); std::sort(margins.begin(), margins.end());
    const int tail_n = std::max(1, static_cast<int>(std::ceil(results.size() * 0.1)));
    const double cvar = std::accumulate(tails.begin(), tails.begin() + tail_n, 0.0) / tail_n;
    const double tail_margin = std::accumulate(margins.begin(), margins.begin() + tail_n, 0.0) / tail_n;
    const double n = results.size();
    file << "],\"win_utility\":" << utility / n << ",\"mean_cash\":" << cash / n
         << ",\"mean_margin\":" << margin / n << ",\"cash_cvar10\":" << cvar
         << ",\"margin_cvar10\":" << tail_margin << ",\"pass_J\":" << 0.8 * cash / n + 0.2 * cvar << "}\n";
    std::printf("%s vs %s games=%zu win=%.4f cash=%.1f margin=%.1f cvar_margin=%.1f seconds=%.3f\n",
                o.a.c_str(), o.b.c_str(), results.size(), utility / n, cash / n, margin / n, tail_margin, seconds);
}

template<class FactoryA, class FactoryB>
int run_batch(const Options& o, FactoryA make_a, FactoryB make_b) {
    const int seats = o.seat_mode == 2 ? 2 : 1;
    const int count = static_cast<int>(o.seeds.size()) * seats;
    std::vector<Outcome> results(count); std::atomic<int> next{0};
    auto worker = [&] {
        auto a = make_a(); auto b = make_b();
        for (;;) {
            const int i = next.fetch_add(1); if (i >= count) break;
            results[i] = run_game(a, b, o.seeds[i / seats], seats == 2 ? i % 2 : o.seat_mode, o);
        }
    };
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int i = 0; i < std::min(o.threads, count); ++i) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    write_results(o, results, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    return 0;
}
}
