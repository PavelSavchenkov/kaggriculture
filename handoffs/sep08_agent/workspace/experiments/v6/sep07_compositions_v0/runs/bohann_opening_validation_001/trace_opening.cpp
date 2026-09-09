#include "registry.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace compositions;

int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path output(argv[1]); std::filesystem::create_directories(output);
    std::ofstream states(output / "states.csv"), actions(output / "actions.csv");
    states << std::setprecision(17);
    states << "agent,opponent,step,day,hour,player,cash,revenue,spend,hires,wheat,carrot,tomato,strawberry,melon,egg,milk,wool,fertilizer,market_wheat,market_milk,market_wool\n";
    actions << "agent,opponent,step,player,workers,markets\n";
    for (const std::string opponent : {"king_rc4", "public_router_v5", "teammate_shoprouter", "crop_mix_t2_wheat"})
    for (const std::string name : {"crop_mix_t2_wheat", "bohann_opening_v1"}) {
        auto own = make_agent(name), rival = make_agent(opponent);
        kag::Config config; config.seed = 1000; kag::Sim sim(config);
        own.reset(kag::agent::runtime::make_agent_init(sim, 0));
        rival.reset(kag::agent::runtime::make_agent_init(sim, 1));
        uint64_t random = config.seed ^ 0xa37108e62d045fb9ULL;
        std::array<uint8_t,8> shops{};
        for (auto& shop : shops) shop = random_word(random) % kag::N_SHOPS;
        auto record = [&] {
            if (sim.st.step > 3 && sim.st.hour != 0 && !sim.st.done) return;
            for (int p = 0; p < 2; ++p) {
                const auto& f = sim.st.farms[p];
                states << name << ',' << opponent << ',' << sim.st.step << ',' << sim.st.day << ',' << sim.st.hour << ','
                       << p << ',' << f.money << ',' << f.sell_revenue << ',' << f.total_spend << ',' << f.hires_today;
                for (int item = 0; item < 9; ++item) states << ',' << f.produced[item];
                states << ',' << sim.st.market.inventory[kag::WHEAT] << ',' << sim.st.market.inventory[kag::MILK]
                       << ',' << sim.st.market.inventory[kag::WOOL] << '\n';
            }
        };
        while (!sim.st.done) {
            std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
            record();
            kag::Action pair[2];
            own.act(kag::agent::runtime::make_observation(sim, 0), {}, pair[0]);
            rival.act(kag::agent::runtime::make_observation(sim, 1), {}, pair[1]);
            for (int p = 0; p < 2; ++p) {
                auto worker = pair[p], market = pair[p]; worker.n_orders = 0; market.n_units = 0;
                uint64_t wh = 14695981039346656037ULL, mh = wh;
                hash_action(wh, worker); hash_action(mh, market);
                actions << name << ',' << opponent << ',' << sim.st.step << ',' << p << ',' << wh << ',' << mh << '\n';
            }
            sim.step(pair[0], pair[1]);
        }
        record();
        std::cout << name << " vs " << opponent << " complete trace\n";
    }
}
