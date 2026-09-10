#include "registry.hpp"
#include "case.hpp"
#include "agents/external/atakan_demand/source/base/agent.hpp"

struct PublicShape {
    std::array<int, kag::N_ITEMS> counts{};
    std::array<int, kag::BOARD * kag::BOARD> tiles{};
    std::array<std::array<int, 13>, kag::BOARD * kag::BOARD> tile_state{};
    std::vector<int> positions, ordered_positions;
    int units = 0, land = 0, hires = 0;
    double cash = 0;
    explicit PublicShape(const kag::agent::PublicFarm& farm) : units(farm.n_units), land(farm.n_quadrants), hires(farm.hires_today), cash(farm.money) {
        for (int y = 0; y < kag::BOARD; ++y) for (int x = 0; x < kag::BOARD; ++x) {
            const auto& tile = farm.tiles[y][x];
            tile_state[y * kag::BOARD + x] = {tile.kind, tile.what, tile.has_animal, tile.watered_today,
                tile.fed_today, tile.cared_today, tile.fertilizer_available, tile.consecutive_dry,
                tile.yield_units, tile.pending_care_bonus, tile.planted_day,
                tile.max_lifespan_step, tile.fertilized_until_day};
            if (tile.has_animal || tile.kind == kag::T_PLANT) {
                ++counts[tile.what]; tiles[y * kag::BOARD + x] = tile.what + 1;
            }
        }
        for (int u = 0; u < farm.n_units; ++u) positions.push_back(farm.pos_y[u] * kag::BOARD + farm.pos_x[u]);
        ordered_positions = positions;
        std::sort(positions.begin(), positions.end());
    }
};

int main(int argc, char** argv) {
    using namespace sales_planner;
    require(argc >= 6, "usage: audit_scenario_start opponent seed_start games output scenario.calendar [...]");
    const std::string opponent = argv[1];
    const uint64_t seed_start = std::stoull(argv[2]); const int games = std::stoi(argv[3]);
    std::ofstream out(argv[4]); require(bool(out), "cannot open audit output");
    struct Source { uint64_t episode; int seat; PublicShape shape; };
    std::vector<Source> sources;
    for (int i = 5; i < argc; ++i) {
        auto c = read_case(argv[i]); kag::Sim sim(c.config);
        while (sim.st.step < 226) {
            const auto& a = c.turns[sim.st.step].original_actions; sim.step(a[0], a[1]);
        }
        for (int seat = 0; seat < 2; ++seat)
            sources.push_back({c.episode, seat, PublicShape(kag::agent::runtime::make_observation(sim, seat ^ 1).opponent())});
    }
    for (int game = 0; game < games; ++game) for (int seat = 0; seat < 2; ++seat) {
        const uint64_t seed = seed_start + game;
        kag::Config config; config.seed = seed; kag::Sim sim(config);
        compositions::atakan_portfolio::Agent own(0); auto rival = make_agent(opponent);
        own.reset(kag::agent::runtime::make_agent_init(sim, seat));
        rival.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
        kag::agent::DecisionBudget budget; budget.max_expansions = 100000;
        uint64_t rng = seed ^ 0xa37108e62d045fb9ULL;
        std::array<uint8_t, 8> shops;
        for (auto& shop : shops) shop = compositions::random_word(rng) % kag::N_SHOPS;
        while (sim.st.step < 226) {
            std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
            kag::Action actions[2];
            const auto a = kag::agent::runtime::make_observation(sim, seat);
            const auto b = kag::agent::runtime::make_observation(sim, seat ^ 1);
            own.act(a, budget, actions[seat]); rival.act(b, budget, actions[seat ^ 1]);
            compositions::validate_action(actions[seat], a); compositions::validate_action(actions[seat ^ 1], b);
            sim.step(actions[0], actions[1]);
        }
        const PublicShape current(kag::agent::runtime::make_observation(sim, seat).opponent());
        for (const auto& source : sources) {
            const auto& s = source.shape;
            out << "{\"opponent\":\"" << opponent << "\",\"seed\":" << seed << ",\"seat\":" << seat
                << ",\"source_episode\":" << source.episode << ",\"source_seat\":" << source.seat
                << ",\"counts_equal\":" << (current.counts == s.counts)
                << ",\"placement_equal\":" << (current.tiles == s.tiles)
                << ",\"workers_equal\":" << (current.units == s.units)
                << ",\"positions_equal\":" << (current.positions == s.positions)
                << ",\"land_equal\":" << (current.land == s.land)
                << ",\"tile_state_equal\":" << (current.tile_state == s.tile_state)
                << ",\"worker_order_equal\":" << (current.ordered_positions == s.ordered_positions)
                << ",\"hiring_equal\":" << (current.hires == s.hires)
                << ",\"cash_equal\":" << (current.cash == s.cash)
                << ",\"current_workers\":" << current.units << ",\"source_workers\":" << s.units
                << ",\"current_land\":" << current.land << ",\"source_land\":" << s.land
                << ",\"current_cash\":" << current.cash << ",\"source_cash\":" << s.cash;
            auto counts = [&](const char* label, const auto& values) {
                out << ",\"" << label << "\":[";
                for (int item = 0; item < kag::N_ITEMS; ++item) { if (item) out << ','; out << values[item]; }
                out << ']';
            };
            counts("current_counts", current.counts); counts("source_counts", s.counts); out << "}\n";
        }
    }
}
