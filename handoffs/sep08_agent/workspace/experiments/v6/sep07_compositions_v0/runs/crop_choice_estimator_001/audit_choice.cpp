#include "choice_value.hpp"
#include "registry.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace compositions;

struct Exact {
    kag::agent::AgentObservation context{};
    uint64_t prefix[2]{14695981039346656037ULL,14695981039346656037ULL};
    double cash[2]{};
    std::array<int,9> produced{},sold{},discarded{};
    uint32_t matched = 0;
};

template<class Agent>
Exact run(Agent& own, AgentBox& rival, uint64_t seed, int seat) {
    kag::Config config; config.seed = seed;
    kag::Sim sim(config);
    own.reset(kag::agent::runtime::make_agent_init(sim, seat));
    rival.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
    uint64_t random = seed ^ 0xa37108e62d045fb9ULL;
    std::array<uint8_t,8> shops{};
    for (auto& shop : shops) shop = random_word(random) % kag::N_SHOPS;
    Exact result;
    while (!sim.st.done) {
        std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
        const auto observation = kag::agent::runtime::make_observation(sim, seat);
        if (observation.day == 12 && observation.hour == 0) result.context = observation;
        kag::Action actions[2];
        own.act(observation, {}, actions[seat]);
        rival.act(kag::agent::runtime::make_observation(sim, seat ^ 1), {}, actions[seat ^ 1]);
        validate_action(actions[seat], observation);
        if (sim.st.day < 12) for (int p = 0; p < 2; ++p) hash_action(result.prefix[p], actions[p]);
        sim.step(actions[0], actions[1]);
    }
    result.matched = own.matched_days();
    for (int p = 0; p < 2; ++p) result.cash[p] = sim.st.farms[seat ^ p].money;
    for (int item = 0; item < 9; ++item) {
        result.produced[item] = sim.st.farms[seat].produced[item];
        result.sold[item] = sim.st.farms[seat].sold_units[item];
        result.discarded[item] = sim.st.farms[seat].discarded[item];
    }
    return result;
}

int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path output(argv[1]); std::filesystem::create_directories(output);
    std::ofstream scores(output / "scores.csv"), products(output / "products.csv"), exact(output / "exact.csv");
    scores << std::setprecision(17); products << std::setprecision(17); exact << std::setprecision(17);
    scores << "opponent,seed,seat,eligible,tomato_shops,model,wheat_own,wheat_rival,tomato_own,tomato_rival,pred_wheat_own,pred_wheat_rival,pred_tomato_own,pred_tomato_rival,microseconds\n";
    products << "opponent,seed,seat,model,product,own_value_delta,rival_value_delta\n";
    exact << "opponent,seed,seat,family,matched_days,product,produced,sold,discarded\n";
    for (const std::string opponent : {"crop_mix_t2_wheat", "wheat_one_fert", "teammate_shoprouter", "public_router", "king_rc4", "public_router_v5"}) {
        auto rival = make_agent(opponent);
        wheat_one_fert::Agent wheat;
        crop_rotation_t2_berry::Course tomato;
        for (uint64_t seed = 1000; seed < 1064; ++seed) for (int seat = 0; seat < 2; ++seat) {
            const std::array<Exact,2> outcomes{run(wheat, rival, seed, seat), run(tomato, rival, seed, seat)};
            for (int p = 0; p < 2; ++p) if (outcomes[0].prefix[p] != outcomes[1].prefix[p]) std::abort();
            const auto& o = outcomes[0].context;
            const bool eligible = outcomes[1].matched & (uint32_t(1) << 12);
            int shops = 0;
            for (int i = 0; i < o.n_shops; ++i) shops += bool(kag::SHOP_MASK[o.shops[i]] & (1u << kag::TOMATO));
            for (int family = 0; family < 2; ++family) for (int item = 0; item < 9; ++item)
                exact << opponent << ',' << seed << ',' << seat << ',' << family << ',' << outcomes[family].matched << ','
                      << item << ',' << outcomes[family].produced[item] << ',' << outcomes[family].sold[item] << ',' << outcomes[family].discarded[item] << '\n';
            if (!eligible) continue;
            for (int model = 0; model < 16; ++model) {
                const auto begin = std::chrono::steady_clock::now();
                const auto prediction = crop_choice::estimate_choice(o, model);
                const double microseconds = std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now() - begin).count();
                scores << opponent << ',' << seed << ',' << seat << ',' << eligible << ',' << shops << ',' << model;
                for (const auto& outcome : outcomes) for (double cash : outcome.cash) scores << ',' << cash;
                for (const auto& value : prediction) scores << ',' << value.own << ',' << value.rival;
                scores << ',' << microseconds << '\n';
                for (int item = 0; item < 9; ++item)
                    products << opponent << ',' << seed << ',' << seat << ',' << model << ',' << item << ','
                             << prediction[1].own_product[item] - prediction[0].own_product[item] << ','
                             << prediction[1].rival_product[item] - prediction[0].rival_product[item] << '\n';
            }
        }
        scores.flush(); products.flush(); exact.flush();
        std::cout << opponent << ": 128 paired leaf comparisons complete\n";
    }
}
