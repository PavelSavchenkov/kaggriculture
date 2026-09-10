#pragma once
#include "lifetimes.hpp"

namespace placement {
inline int cold_land_purchases(const std::string& name, int day) {
    return name == "expanding_mixed" && (day == 9 || day == 16);
}

inline std::vector<LifeProgram> cold_case(const std::string& name) {
    std::vector<LifeProgram> lives;
    auto add = [&](int item, int count, int begin, int stop = 30) {
        auto spec = productive_life(item, begin, stop);
        if (name == "expanding_mixed") for (int cell = 0; cell < 100; ++cell)
            spec.allowed[cell] = quadrant(cell) < 1 + (begin >= 9) + (begin >= 16);
        const auto program = compile_life(spec);
        for (int i = 0; i < count; ++i) lives.push_back(program);
    };
    int wheat = 0;
    if (name == "wheat12") wheat = 12;
    else if (name == "dairy") { wheat = 8; add(kag::COW, 2, 4); }
    else if (name == "mixed_animals") { wheat = 8; add(kag::COW, 2, 4); add(kag::SHEEP, 2, 4); }
    else if (name == "geese") { wheat = 10; add(kag::GOOSE, 4, 4); }
    else if (name == "crop_rotation") { wheat = 6; add(kag::MELON, 4, 0); add(kag::STRAWBERRY, 4, 12); }
    else if (name == "expanding_mixed") {
        add(kag::COW, 2, 4); add(kag::SHEEP, 2, 4); add(kag::COW, 4, 10); add(kag::SHEEP, 4, 13); add(kag::GOOSE, 6, 18);
        for (int d = 0; d <= 24; d += 4) add(kag::WHEAT, d < 12 ? 12 : d < 16 ? 20 : 36, d);
        return lives;
    }
    else throw std::runtime_error("unknown cold case");
    // Harvest and replacement occur on the same day. Individual lifetimes,
    // rather than fixed tile histories, decide whether to reuse the cell.
    for (int d = 0; d <= 24; d += 4) add(kag::WHEAT, wheat, d);
    return lives;
}

inline LifeAssignment read_life_assignment(const fs::path& path, size_t count) {
    std::ifstream input(path); LifeAssignment result(count);
    for (int& cell : result) if (!(input >> cell)) throw std::runtime_error("short life assignment");
    std::string extra; if (input >> extra) throw std::runtime_error("extra life assignment content");
    return result;
}

inline void save_life_assignment(const std::vector<LifeProgram>& lives, const LifeAssignment& assignment, const fs::path& folder) {
    std::ofstream table(folder / "lives.csv"), cells(folder / "assignment.txt");
    table << "id,item,begin_day,stop_day,release_day,cell,water,feed,care,collect,harvest,fertilize,clear_after_service,allowed_cells\n";
    for (size_t i = 0; i < lives.size(); ++i) {
        const auto& p = lives[i]; const auto& s = p.spec;
        table << i << ',' << s.item << ',' << s.begin_day << ',' << s.stop_day << ',' << p.release_day << ',' << assignment[i]
              << ',' << s.water << ',' << s.feed << ',' << s.care << ',' << s.collect << ',' << s.harvest << ',' << s.fertilize << ',' << s.clear_after_service << ',';
        for (bool allowed : s.allowed) table << allowed;
        table << '\n';
        cells << assignment[i] << ' ';
    }
    cells << '\n';
}

struct PlacementProgram { std::vector<LifeProgram> lives; std::array<int, 30> land{}; };

inline PlacementProgram load_placement_program(const std::string& name) {
    PlacementProgram result;
    if (!fs::is_regular_file(name)) {
        result.lives = cold_case(name);
        for (int d = 0; d < 30; ++d) result.land[d] = cold_land_purchases(name, d);
        return result;
    }
    std::ifstream input(name); std::string version; int count = -1;
    input >> version >> count;
    if ((version != "PLACEMENT_LIVES1" && version != "PLACEMENT_LIVES2" && version != "PLACEMENT_LIVES3" && version != "PLACEMENT_LIVES4") || count < 0 || count > 10000) throw std::runtime_error("invalid placement-program header");
    int quadrants = 1;
    for (int& purchase : result.land) {
        input >> purchase; quadrants += purchase;
        if (!input || purchase < 0 || quadrants > 4) throw std::runtime_error("invalid land calendar");
    }
    // The oracle depends only on this immutable specification. Preserve input
    // order and independent program values, while compiling each exact spec once.
    result.lives.reserve(count);
    std::unordered_map<LifeSpec, size_t, LifeSpecHash> compiled;
    for (int i = 0; i < count; ++i) {
        LifeSpec spec{}; std::string allowed;
        input >> spec.item >> spec.begin_day >> spec.stop_day >> spec.water >> spec.feed >> spec.care >> spec.collect >> spec.harvest >> spec.fertilize >> allowed;
        if (version != "PLACEMENT_LIVES1") {
            int after = -1; input >> after;
            if (after != 0 && after != 1) throw std::runtime_error("invalid clear timing");
            spec.clear_after_service = after;
        }
        if (version == "PLACEMENT_LIVES3" || version == "PLACEMENT_LIVES4") for (auto& order : spec.service_order) input >> order;
        if (version == "PLACEMENT_LIVES4") {
            int repeated = -1; input >> repeated;
            if (repeated != 0 && repeated != 1) throw std::runtime_error("invalid repeated-service flag");
            spec.repeated_service = repeated;
        }
        if (!input || allowed.size() != 100) throw std::runtime_error("invalid life record");
        for (int cell = 0; cell < 100; ++cell) {
            if (allowed[cell] != '0' && allowed[cell] != '1') throw std::runtime_error("invalid allowed-cell mask");
            spec.allowed[cell] = allowed[cell] == '1';
        }
        if (const auto found = compiled.find(spec); found != compiled.end()) result.lives.push_back(result.lives[found->second]);
        else {
            compiled.emplace(spec, result.lives.size());
            result.lives.push_back(compile_life(spec));
        }
    }
    std::string extra; if (input >> extra) throw std::runtime_error("extra placement-program content");
    return result;
}

inline void save_placement_program(const PlacementProgram& program, const fs::path& path) {
    const bool extended = std::any_of(program.lives.begin(), program.lives.end(), [](const auto& life) { return life.spec.clear_after_service; });
    const bool ordered = std::any_of(program.lives.begin(), program.lives.end(), [](const auto& life) { return std::any_of(life.spec.service_order.begin(), life.spec.service_order.end(), [](auto value) { return value != 0; }); });
    const bool repeated = std::any_of(program.lives.begin(), program.lives.end(), [](const auto& life) { return life.spec.repeated_service; });
    std::ofstream output(path); output << (repeated ? "PLACEMENT_LIVES4 " : ordered ? "PLACEMENT_LIVES3 " : extended ? "PLACEMENT_LIVES2 " : "PLACEMENT_LIVES1 ") << program.lives.size() << '\n';
    for (int count : program.land) output << count << ' ';
    output << '\n';
    for (const auto& life : program.lives) {
        const auto& s = life.spec;
        output << s.item << ' ' << s.begin_day << ' ' << s.stop_day << ' ' << s.water << ' ' << s.feed << ' ' << s.care << ' ' << s.collect << ' ' << s.harvest << ' ' << s.fertilize << ' ';
        for (bool allowed : s.allowed) output << allowed;
        if (extended || ordered || repeated) output << ' ' << s.clear_after_service;
        if (ordered || repeated) for (auto order : s.service_order) output << ' ' << order;
        if (repeated) output << ' ' << s.repeated_service;
        output << '\n';
    }
}
}
