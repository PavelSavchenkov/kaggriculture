#include "library/source/agent.hpp"
#include "registry.hpp"
#include <filesystem>

int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path directory(argv[1]);
    std::filesystem::create_directories(directory);
    using namespace compositions;
    Options options; options.games = 32; options.threads = 8; options.validate = true;
    for (uint64_t seed = 1000; seed < 1032; ++seed) options.seeds.push_back(seed);
    for (const std::string opponent : {"crop_mix_t2_wheat", "wheat_one_fert", "teammate_shoprouter", "public_router", "king_rc4", "public_router_v5"}) {
        options.b = opponent;
        options.a = "crop_mix_t2_wheat";
        options.output = (directory / ("current_vs_" + opponent + ".json")).string();
        run_batch(options, [] { return crop_mix_t2_wheat::Agent{}; }, [&] { return make_agent(opponent); });
        for (int program = 0; program < fresh_courses_1642::program_count(); ++program) {
            options.a = "fresh_course_" + std::to_string(program);
            options.output = (directory / (options.a + "_vs_" + opponent + ".json")).string();
            run_batch(options, [=] { return fresh_courses_1642::Agent(program); }, [&] { return make_agent(opponent); });
        }
    }
}
