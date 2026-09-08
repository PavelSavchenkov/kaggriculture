#include "policy/source/agent.hpp"
#include "registry.hpp"
#include <filesystem>
#include <set>

int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path output(argv[1]); std::filesystem::create_directories(output);
    using namespace compositions;
    std::vector<std::array<int,3>> choices{{81,13,0},{0,30,2},{0,13,2}};
    for (int quantity : {32,48,64,80,81,96}) choices.push_back({quantity,30,1});
    for (int quantity : {32,64,80,81,96}) choices.push_back({quantity,13,2});
    for (int buffer : {5,8,10,20,30}) choices.push_back({81,buffer,2});
    std::ofstream table(output / "choices.csv"); table << "name,quantity,buffer,mode\n";
    for (const auto& choice : choices) table << "q" << choice[0] << "_b" << choice[1] << "_m" << choice[2]
                                            << ',' << choice[0] << ',' << choice[1] << ',' << choice[2] << '\n';
    table.close();
    Options options; options.games = 64; options.threads = 8; options.validate = true;
    for (uint64_t seed = 1000; seed < 1064; ++seed) options.seeds.push_back(seed);
    for (const std::string opponent : {"crop_mix_t2_wheat", "wheat_one_fert", "teammate_shoprouter", "public_router", "king_rc4", "public_router_v5"}) {
        options.b = opponent;
        for (const std::string parent : {"crop_mix_t2_wheat", "bohann_opening_v1"}) {
            options.a = parent; options.output = (output / (parent + "_vs_" + opponent + ".json")).string();
            run_batch(options, [&] { return make_agent(parent); }, [&] { return make_agent(opponent); });
        }
        for (const auto& choice : choices) {
            options.a = "q" + std::to_string(choice[0]) + "_b" + std::to_string(choice[1]) + "_m" + std::to_string(choice[2]);
            options.output = (output / (options.a + "_vs_" + opponent + ".json")).string();
            run_batch(options, [=] { return opening_market_search::Agent(choice[0],choice[1],choice[2]); }, [&] { return make_agent(opponent); });
        }
    }
}
