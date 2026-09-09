#include "agents/external/early_structure_cow/source/agent.hpp"
#include "agents/external/nanare_four_quadrant_course/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <future>
#include <iostream>
#include <variant>

using Early = kag::agents::early_structure_cow::Agent;
using Course = kag::agents::nanare_four_quadrant_course::Agent;
static_assert(kag::agent::LocalAgent<Early> && kag::agent::LocalAgent<Course>);

struct Pass {
    static kag::agent::AgentInfo info() { return {"pass"}; }
    void reset(const kag::agent::AgentInit&) {}
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget&, kag::Action& action) {
        action.clear();
        action.n_units = observation.self().n_units;
        for (int i = 0; i < action.n_units; ++i) action.units[i] = {};
        action.finalize();
    }
};

struct Generic {
    std::variant<Early, Course, Pass> policy;
    explicit Generic(int id) {
        if (id == 0) policy.emplace<Early>();
        else if (id == 1) policy.emplace<Course>();
        else { assert(id == 2); policy.emplace<Pass>(); }
    }
    void reset(const kag::agent::AgentInit& init) {
        std::visit([&](auto& p) { p.reset(init); }, policy);
    }
    void act(const kag::agent::AgentObservation& observation,
             const kag::agent::DecisionBudget& budget, kag::Action& action) {
        std::visit([&](auto& p) { p.act(observation, budget, action); }, policy);
    }
};

struct Result {
    std::array<uint64_t, 2> hashes{14695981039346656037ULL, 14695981039346656037ULL};
    std::array<double, 2> cash{};
    bool operator==(const Result&) const = default;
};

void inspect(const kag::Action& action, const kag::agent::AgentObservation& observation, uint64_t& hash) {
    assert(action.n_units == observation.self().n_units);
    assert(action.n_orders >= 0 && action.n_orders <= 10 && action.metadata_ready);
    auto expected = action;
    expected.finalize();
    assert(expected.plant_mask == action.plant_mask);
    for (int i = 0; i < kag::N_CROPS; ++i) assert(expected.plant_demand[i] == action.plant_demand[i]);
    auto add = [&](int value) { hash ^= static_cast<uint32_t>(value); hash *= 1099511628211ULL; };
    add(action.n_units); add(action.n_orders);
    for (int i = 0; i < action.n_units; ++i) {
        assert(action.units[i].op < kag::OP_INVALID);
        add(action.units[i].op); add(action.units[i].arg); add(action.units[i].n);
    }
    for (int i = 0; i < action.n_orders; ++i) {
        add(action.orders[i].op); add(action.orders[i].item); add(action.orders[i].n);
    }
}

template<class A, class B>
Result play(A& a, B& b, uint64_t seed, int seat) {
    kag::Config config;
    config.seed = seed;
    kag::Sim sim(config);
    a.reset(kag::agent::runtime::make_agent_init(sim, seat));
    b.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
    kag::agent::DecisionBudget budget;
    budget.max_expansions = 100000;
    Result result;
    while (!sim.st.done) {
        const auto oa = kag::agent::runtime::make_observation(sim, seat);
        const auto ob = kag::agent::runtime::make_observation(sim, seat ^ 1);
        kag::Action actions[2];
        a.act(oa, budget, actions[seat]); b.act(ob, budget, actions[seat ^ 1]);
        inspect(actions[seat], oa, result.hashes[seat]);
        inspect(actions[seat ^ 1], ob, result.hashes[seat ^ 1]);
        sim.step(actions[0], actions[1]);
    }
    assert(sim.st.step == 719);
    for (int p = 0; p < 2; ++p) result.cash[p] = sim.st.farms[p].money;
    return result;
}

template<class A, class B>
void check(int id_a, int id_b, uint64_t seed, int seat) {
    auto run = [&] {
#ifdef FAST_ESTIMATOR_TYPED_FIXTURE
        A a; B b;
#else
        Generic a(id_a), b(id_b);
#endif
        const auto result = play(a, b, seed, seat);
        assert(result == play(a, b, seed, seat)); // Reused instances must reset.
        return result;
    };
    auto parallel = std::async(std::launch::async, run);
    const auto result = run();
    assert(result == parallel.get());
    std::cout << id_a << ' ' << id_b << ' ' << seed << ' ' << seat << ' '
              << result.hashes[0] << ' ' << result.hashes[1] << ' '
              << result.cash[0] << ' ' << result.cash[1] << '\n';
}

int main() {
    for (const uint64_t seed : {1201301738ULL, 1470762556ULL}) {
        for (int seat = 0; seat < 2; ++seat) {
            check<Early, Course>(0, 1, seed, seat);
            check<Early, Early>(0, 0, seed, seat);
            check<Course, Course>(1, 1, seed, seat);
            check<Early, Pass>(0, 2, seed, seat);
            check<Course, Pass>(1, 2, seed, seat);
        }
    }
}
