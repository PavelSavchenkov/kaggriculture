#include "cold_cases.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if ((argc != 2 && argc != 3) || fs::exists(argv[1])) throw std::runtime_error("usage: generate_cases new_output [service]");
        const fs::path output(argv[1]); fs::create_directories(output);
        std::ofstream index(output / "cases.csv"); index << "name,description,lives\n";
        auto save = [&](const std::string& name, const std::string& description, const PlacementProgram& p) {
            save_placement_program(p, output / (name + ".plan"));
            const auto assignment = assign_lives(p.lives, true, true);
            const auto folder = output / name; fs::create_directories(folder); save_life_assignment(p.lives, assignment, folder);
            index << name << ',' << description << ',' << p.lives.size() << '\n';
        };
        auto add = [](PlacementProgram& p, int item, int count, int day, bool low_care = false) {
            auto spec = productive_life(item, day);
            if (low_care) spec.care = 0x15555555;
            const auto life = compile_life(spec); for (int i = 0; i < count; ++i) p.lives.push_back(life);
        };
        auto wheat = [&](PlacementProgram& p, int count, int first = 0) { for (int d = first; d <= 26; d += 4) add(p, kag::WHEAT, count, d); };
        if (argc == 3) {
            if (std::string(argv[2]) != "service") throw std::runtime_error("unknown case family");
            for (int count : {8, 14, 20}) {
                PlacementProgram p; add(p, kag::COW, 2, 4);
                for (int d = 0; d <= 24; d += 4) {
                    auto spec = productive_life(kag::WHEAT, d);
                    if (d >= 8) spec.fertilize = uint32_t{1} << (d + 2);
                    const auto life = compile_life(spec);
                    for (int i = 0; i < count; ++i) p.lives.push_back(life);
                }
                save("fertilized_wheat" + std::to_string(count), "two cows and late wheat fertilized at age two", p);
            }
            for (int item : {kag::CARROT, kag::WHEAT, kag::MELON}) {
                PlacementProgram p;
                const int period = kag::CROPS[item].max_yield_day;
                for (int d = 0; d + period < 30; d += period) {
                    auto spec = productive_life(item, d);
                    spec.water = 0;
                    for (int t = d; t <= d + period; t += 2) spec.water |= uint32_t{1} << t;
                    const auto life = compile_life(spec);
                    for (int i = 0; i < 14; ++i) p.lives.push_back(life);
                }
                save("sparse_crop" + std::to_string(item), "fourteen crops watered on alternate days", p);
            }
            for (bool stagger : {false, true}) {
                PlacementProgram p; wheat(p, 10); add(p, kag::COW, 2, 4); add(p, kag::SHEEP, 2, 4);
                for (int i = 0; i < 6; ++i) {
                    auto spec = productive_life(kag::TOMATO, 0);
                    spec.water = (stagger && (i & 1)) ? 0x2aaaaaaa : 0x15555555;
                    p.lives.push_back(compile_life(spec));
                }
                save(stagger ? "sparse_tomato_stagger" : "sparse_tomato_aligned", "sparse ongoing crop service with mixed animals", p);
            }
            std::cout << "8 service calendar inputs written\n";
            return 0;
        }
        for (int count : {9, 17, 23}) { PlacementProgram p; wheat(p, count); save("wheat" + std::to_string(count), "changed repeated wheat count", p); }
        { PlacementProgram p; for (int d = 0; d <= 27; d += 3) add(p, kag::CARROT, 10, d); save("carrot10", "ten carrots every three days", p); }
        { PlacementProgram p; wheat(p, 7); add(p, kag::TOMATO, 6, 0); add(p, kag::TOMATO, 6, 13); save("tomato_rotation", "two tomato lives and repeated wheat", p); }
        { PlacementProgram p; wheat(p, 7); add(p, kag::STRAWBERRY, 5, 0); add(p, kag::STRAWBERRY, 5, 18); save("strawberry_rotation", "strawberry renewal and repeated wheat", p); }
        { PlacementProgram p; wheat(p, 7); add(p, kag::MELON, 8, 0); add(p, kag::MELON, 8, 12); add(p, kag::WHEAT, 8, 24); save("melon_wheat", "melon chain followed by wheat", p); }
        for (bool low_care : {false, true}) {
            PlacementProgram p; wheat(p, 10); add(p, kag::COW, 3, 5, low_care); add(p, kag::SHEEP, 2, 9, low_care);
            save(low_care ? "mixed_low_care" : "mixed_delayed", low_care ? "alternate-day animal care" : "different cow and sheep birth dates", p);
        }
        { PlacementProgram p; wheat(p, 14); add(p, kag::GOOSE, 6, 6); save("geese6", "six later geese with fourteen wheat", p); }
        { PlacementProgram p; wheat(p, 10); wheat(p, 10, 2); save("staggered_wheat", "two ten-wheat cohorts two days apart", p); }
        {
            auto p = load_placement_program("expanding_mixed");
            std::array<int, 30> retained{};
            std::erase_if(p.lives, [&](const LifeProgram& life) {
                return life.spec.item == kag::WHEAT && life.spec.begin_day >= 16 && ++retained[life.spec.begin_day] > 32;
            });
            save("expanding32", "late wheat count reduced from thirty-six to thirty-two", p);
        }
        std::cout << "12 new composition inputs written\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
