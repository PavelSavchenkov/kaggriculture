#include "agent.hpp"
#include "../tests/pass/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using Policy = kag::agents::day_policy_contract::Agent;
using Pass = kag::agents::day_policy_test_pass::Agent;
using namespace kag;
struct RuntimeAgent {
    virtual ~RuntimeAgent() = default;
    virtual void reset(const agent::AgentInit&) = 0;
    virtual void act(const agent::AgentObservation&, const agent::DecisionBudget&, Action&) = 0;
};
template<class T> struct Adapter final : RuntimeAgent {
    T policy;
    void reset(const agent::AgentInit& init) override { policy.reset(init); }
    void act(const agent::AgentObservation& o, const agent::DecisionBudget& b, Action& a) override { policy.act(o, b, a); }
};
struct Dynamic {
    std::unique_ptr<RuntimeAgent> policy;
    explicit Dynamic(const std::string& name) {
        if (name == "day_policy_contract") policy = std::make_unique<Adapter<Policy>>();
        else if (name == "pass") policy = std::make_unique<Adapter<Pass>>();
        else throw std::runtime_error("unknown arena agent");
    }
    void reset(const agent::AgentInit& init) { policy->reset(init); }
    void act(const agent::AgentObservation& o, const agent::DecisionBudget& b, Action& a) { policy->act(o, b, a); }
};
struct Outcome {
    uint64_t seed = 0, hash[2] = {14695981039346656037ull, 14695981039346656037ull};
    double cash[2]{};
    int turns = 0, faults = 0, seat = 0;
};
void hash(uint64_t& h, const Action& a) {
    auto add = [&](int x) { h ^= uint32_t(x); h *= 1099511628211ull; };
    add(a.n_units); add(a.n_orders);
    for (int u = 0; u < a.n_units; ++u) { add(a.units[u].op); add(a.units[u].arg); add(a.units[u].n); }
    for (int k = 0; k < a.n_orders; ++k) { add(a.orders[k].op); add(a.orders[k].item); add(a.orders[k].n); }
}
template<class A, class B>
Outcome run(A& first, B& second, uint64_t seed, int seat, uint64_t expansions, bool validate) {
    Config cfg; cfg.seed = seed; Sim sim(cfg);
    first.reset(agent::runtime::make_agent_init(sim, seat));
    second.reset(agent::runtime::make_agent_init(sim, seat ^ 1));
    Outcome result; result.seed = seed; result.seat = seat;
    agent::DecisionBudget budget; budget.max_expansions = expansions;
    while (!sim.st.done) {
        Action a[2];
        const auto own = agent::runtime::make_observation(sim, seat);
        const auto other = agent::runtime::make_observation(sim, seat ^ 1);
        first.act(own, budget, a[seat]); second.act(other, budget, a[seat ^ 1]);
        for (int p = 0; p < 2; ++p) {
            if (a[p].n_units != sim.st.farms[p].n_units || a[p].n_orders > 10 || !a[p].metadata_ready)
                throw std::runtime_error("arena action contract");
            if (validate) {
                auto expected = a[p]; expected.finalize();
                if (expected.plant_mask != a[p].plant_mask) throw std::runtime_error("plant mask mismatch");
                for (int crop = 0; crop < N_CROPS; ++crop)
                    if (expected.plant_demand[crop] != a[p].plant_demand[crop]) throw std::runtime_error("seed metadata mismatch");
                auto outcome = sim.diagnose_solo_action(p, a[p], true);
                result.faults += outcome.requested_unit_actions - outcome.successful_unit_actions;
            }
            hash(result.hash[p], a[p]);
        }
        sim.step(a[0], a[1]); ++result.turns;
    }
    result.cash[0] = sim.reward(0); result.cash[1] = sim.reward(1);
    return result;
}
template<class A, class B>
Outcome run_pair_batch(uint64_t seed, int seat, uint64_t expansions, bool validate) {
    A first; B second; return run(first, second, seed, seat, expansions, validate);
}

int main(int argc, char** argv) {
    std::string first = "day_policy_contract", second = "pass", output;
    uint64_t seed_start = 1000, expansions = 256;
    int games = 8, threads = 1, seat_mode = 2;
    bool validate = false;
    std::vector<uint64_t> seeds;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        if (key == "--validate") { validate = true; continue; }
        if (++i >= argc) throw std::runtime_error("missing option value");
        std::string value = argv[i];
        if (key == "--a") first = value;
        else if (key == "--b") second = value;
        else if (key == "--output") output = value;
        else if (key == "--games") games = std::stoi(value);
        else if (key == "--threads") threads = std::stoi(value);
        else if (key == "--seed-start") seed_start = std::stoull(value);
        else if (key == "--budget-expansions") expansions = std::stoull(value);
        else if (key == "--seat-mode") seat_mode = value == "both" ? 2 : std::stoi(value);
        else if (key == "--seed-file") {
            std::ifstream file(value); uint64_t seed;
            while (file >> seed) seeds.push_back(seed);
            if (!file.eof() || seeds.empty()) throw std::runtime_error("bad seed file");
        } else throw std::runtime_error("unknown arena option");
    }
    if (threads < 1 || games < 1 || seat_mode < 0 || seat_mode > 2 || output.empty()) throw std::runtime_error("invalid arena options");
    if (seeds.empty()) for (int i = 0; i < games; ++i) seeds.push_back(seed_start + i);
    std::vector<Outcome> outcomes(seeds.size() * (seat_mode == 2 ? 2 : 1));
    std::atomic<size_t> next = 0;
    auto worker = [&] {
        for (;;) {
            const auto index = next.fetch_add(1);
            if (index >= outcomes.size()) break;
            const int seat = seat_mode == 2 ? int(index % 2) : seat_mode;
            const uint64_t seed = seeds[seat_mode == 2 ? index / 2 : index];
#ifdef PAIR_RUNNER
            if (first != "day_policy_contract") throw std::runtime_error("unsupported concrete first agent");
            if (second == "pass") outcomes[index] = run_pair_batch<Policy, Pass>(seed, seat, expansions, validate);
            else if (second == "day_policy_contract") outcomes[index] = run_pair_batch<Policy, Policy>(seed, seat, expansions, validate);
            else throw std::runtime_error("unsupported concrete second agent");
#else
            Dynamic a(first), b(second); outcomes[index] = run(a, b, seed, seat, expansions, validate);
#endif
        }
    };
    std::vector<std::thread> workers;
    for (int i = 0; i < threads; ++i) workers.emplace_back(worker);
    for (auto& thread : workers) thread.join();
    std::ofstream out(output); out << "[\n";
    for (size_t i = 0; i < outcomes.size(); ++i) {
        const auto& r = outcomes[i];
        out << (i ? ",\n" : "") << "{\"seed\":" << r.seed << ",\"seat\":" << r.seat << ",\"turns\":" << r.turns
            << ",\"faults\":" << r.faults << ",\"cash\":[" << r.cash[0] << ',' << r.cash[1]
            << "],\"hash\":[" << r.hash[0] << ',' << r.hash[1] << "]}";
    }
    out << "\n]\n";
    if (!out) throw std::runtime_error("arena output failed");
}
