#include "baselines/warm/runs/submission_losses_sep08_001/general_quadrant_compile/source/season.hpp"
#include "actions.hpp"
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    const std::filesystem::path course = argv[1], output = argv[3];
    if (std::filesystem::exists(output)) return 2;
    Config config; config.seed = std::stoull(argv[2]);
    Sim sim(config); Source own; ContextRival rival;
    own.reset(agent::runtime::make_agent_init(sim, 0));
    rival.reset(agent::runtime::make_agent_init(sim, 1));
    std::array<std::array<Action, 24>, 30> replacement{};
    std::array<bool, 30> replaced{};
    for (int day = 0; day < 30; ++day) {
        const auto path = course / "days" / std::to_string(day) / "actions.txt";
        if (std::filesystem::is_regular_file(path)) {
            replacement[day] = labor::offline::read_actions(path.string()); replaced[day] = true;
        }
    }
    constexpr std::array<uint8_t, 8> shops{5, 2, 5, 5, 5, 4, 0, 1};
    uint64_t hashes[2]{14695981039346656037ULL, 14695981039346656037ULL};
    int hires = 0, hiring_cost = 0;
    while (!sim.st.done) {
        std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
        Action pair[2];
        own.act(agent::runtime::make_observation(sim, 0), decision_budget(), pair[0]);
        rival.act(agent::runtime::make_observation(sim, 1), decision_budget(), pair[1]);
        if (replaced[sim.st.day]) pair[0] = replacement[sim.st.day][sim.st.hour];
        for (int p = 0; p < 2; ++p) {
            validate_action(pair[p], agent::runtime::make_observation(sim, p)); hash_action(hashes[p], pair[p]);
        }
        const auto phase = prefix_phase(sim, pair, 10);
        const int before = sim.st.farms[0].hires_today, after = phase.st.farms[0].hires_today;
        for (int h = before; h < after; ++h) { ++hires; hiring_cost += config.hire_mult * kag::fib(h); }
        sim.step(pair[0], pair[1]);
    }
    if (sim.st.step != 719) std::abort();
    std::ofstream report(output);
    report << "{\"real_transitions\":719,\"cash\":" << sim.st.farms[0].money << ",\"rival_cash\":" << sim.st.farms[1].money
           << ",\"action_hashes\":[" << hashes[0] << ',' << hashes[1] << "],\"hires\":" << hires << ",\"hire_cost\":" << hiring_cost;
    for (int player = 0; player < 2; ++player) {
        report << ",\"produced" << player << "\":[";
        for (int item = 0; item < N_ITEMS; ++item) report << (item ? "," : "") << sim.st.farms[player].produced[item];
        report << ']';
    }
    report << "}\n";
}
