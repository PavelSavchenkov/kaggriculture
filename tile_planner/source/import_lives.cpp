#include "cold_cases.hpp"
#include <tuple>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3 || fs::exists(argv[2])) throw std::runtime_error("usage: import_lives source_course new_output");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]); const fs::path output(argv[2]); fs::create_directories(output);
        struct Record { LifeSpec spec; int cell; std::array<std::vector<day_solver::TileWorkAction>, 30> work; };
        std::vector<Record> records;
        std::array<int, 100> active; active.fill(-1);
        std::array<std::array<int, 100>, 30> dawn_owner;
        PlacementProgram program;
        int quadrants = 1, ignored_builds = 0, independent_clears = 0;
        for (int d = 0; d < 30; ++d) {
            const auto& day = course.days[d];
            for (const auto& event : day.problem.market_plan)
                if (event.market_op == kag::M_BUY_LAND) program.land[d] += event.quantity;
            quadrants += program.land[d];
            if (quadrants > 4) throw std::runtime_error("unexpected repeated land commitment");
            for (int cell = 0; cell < 100; ++cell) {
                const bool alive = living(day.problem.start.managed_tiles[cell].state);
                if (!alive) active[cell] = -1;
                if (alive && active[cell] < 0) throw std::runtime_error("untracked dawn life at " + std::to_string(d) + ":" + std::to_string(cell));
            }
            dawn_owner[d] = active;
            for (const auto& work : day.problem.tile_work) {
                const int cell = work.tile;
                for (const auto& action : work.actions) {
                    if (action.op == kag::OP_BUILD_COOP || action.op == kag::OP_BUILD_PASTURE) { ++ignored_builds; continue; }
                    if (action.op == kag::OP_PLANT || action.op == kag::OP_PLACE) {
                        if (active[cell] >= 0 || (action.op == kag::OP_PLACE && !kag::is_animal(action.arg))) throw std::runtime_error("unsupported overlapping birth");
                        LifeSpec spec{action.arg, d, 30};
                        spec.water = spec.feed = spec.care = spec.collect = spec.harvest = spec.fertilize = 0;
                        for (int target = 0; target < 100; ++target) spec.allowed[target] = quadrant(target) < quadrants;
                        active[cell] = records.size(); records.push_back({spec, cell, {}});
                    }
                    if (action.op == kag::OP_DIG) {
                        if (active[cell] < 0) { ++independent_clears; continue; }
                        auto& record = records[active[cell]];
                        record.spec.clear_after_service = !record.work[d].empty();
                        record.spec.stop_day = d; record.work[d].push_back(action); active[cell] = -1; continue;
                    }
                    if (active[cell] < 0) throw std::runtime_error("biological action has no life");
                    auto& record = records[active[cell]]; auto& spec = record.spec;
                    uint32_t* mask = nullptr;
                    if (action.op == kag::OP_WATER) mask = &spec.water;
                    else if (action.op == kag::OP_FEED) mask = &spec.feed;
                    else if (action.op == kag::OP_CARE) mask = &spec.care;
                    else if (action.op == kag::OP_COLLECT_FERTILIZER) mask = &spec.collect;
                    else if (action.op == kag::OP_HARVEST) mask = &spec.harvest;
                    else if (action.op == kag::OP_FERTILIZE) mask = &spec.fertilize;
                    else if (action.op != kag::OP_PLANT && action.op != kag::OP_PLACE) throw std::runtime_error("unsupported biological action");
                    if (mask) {
                        if (*mask & (uint32_t{1} << d)) spec.repeated_service = true;
                        *mask |= uint32_t{1} << d;
                    }
                    record.work[d].push_back(action);
                    if (action.op == kag::OP_HARVEST && kag::is_crop(spec.item) && !kag::CROPS[spec.item].ongoing) active[cell] = -1;
                }
            }
        }
        using WorkKey = std::tuple<int, int, int, int64_t>;
        auto normalized = [](const std::vector<day_solver::TileWorkAction>& actions) {
            std::vector<WorkKey> result;
            for (const auto& a : actions) if (a.op != kag::OP_BUILD_COOP && a.op != kag::OP_BUILD_PASTURE)
                result.emplace_back(a.op, a.arg, a.output_item, a.output_quantity);
            std::sort(result.begin(), result.end()); return result;
        };
        std::ofstream errors(output / "errors.txt");
        int state_checks = 0, work_checks = 0, mismatches = 0;
        LifeAssignment assignment;
        for (size_t id = 0; id < records.size(); ++id) {
            auto& record = records[id];
            for (int d = 0; d < 30; ++d) {
                std::vector<int> codes;
                for (const auto& a : record.work[d]) {
                    if (a.op == kag::OP_WATER) codes.push_back(1);
                    if (a.op == kag::OP_FEED) codes.push_back(2);
                    if (a.op == kag::OP_CARE) codes.push_back(3);
                    if (a.op == kag::OP_COLLECT_FERTILIZER) codes.push_back(4);
                    if (a.op == kag::OP_HARVEST) codes.push_back(5);
                    if (a.op == kag::OP_FERTILIZE) codes.push_back(6);
                }
                auto canonical = codes;
                std::sort(canonical.begin(), canonical.end(), [](int a, int b) { return (a == 6 ? 0 : a) < (b == 6 ? 0 : b); });
                if (codes.size() > 8) throw std::runtime_error("more than eight service operations in one life-day");
                if (codes != canonical || record.spec.repeated_service) for (size_t i = 0; i < codes.size(); ++i) record.spec.service_order[d] |= uint32_t(codes[i]) << (4 * i);
            }
            const auto life = compile_life(record.spec);
            for (int d = 0; d < 30; ++d) {
                if (dawn_owner[d][record.cell] == int(id)) {
                    ++state_checks;
                    if (life.days[d].before != course.days[d].problem.start.managed_tiles[record.cell].state) {
                        const auto& a = life.days[d].before; const auto& b = course.days[d].problem.start.managed_tiles[record.cell].state;
                        ++mismatches; errors << "state " << id << ' ' << d << ' ' << record.cell << " oracle/source stored " << a.stored_units << '/' << b.stored_units
                            << " dry " << a.consecutive_dry_days << '/' << b.consecutive_dry_days << " fertilizer " << a.fertilizer_days_remaining << '/' << b.fertilizer_days_remaining
                            << " care " << a.pending_care_bonus << '/' << b.pending_care_bonus << '\n';
                    }
                }
                ++work_checks;
                if (normalized(life.days[d].work) != normalized(record.work[d])) {
                    ++mismatches; errors << "work " << id << ' ' << d << ' ' << record.cell << '\n';
                    for (const auto& [op, arg, item, quantity] : normalized(life.days[d].work)) errors << "oracle " << op << ' ' << arg << ' ' << item << ' ' << quantity << '\n';
                    for (const auto& [op, arg, item, quantity] : normalized(record.work[d])) errors << "source " << op << ' ' << arg << ' ' << item << ' ' << quantity << '\n';
                }
            }
            program.lives.push_back(life); assignment.push_back(record.cell);
        }
        const bool legal = legal_lives(program.lives, assignment);
        if (!legal) { ++mismatches; errors << "source assignment fails lifetime occupancy\n"; }
        if (!mismatches) {
            save_placement_program(program, output / "INPUT.plan"); save_life_assignment(program.lives, assignment, output);
        } else {
            fs::create_directories(output / "diagnostic");
            save_placement_program(program, output / "diagnostic/UNVERIFIED.plan");
            save_life_assignment(program.lives, assignment, output / "diagnostic");
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"biological_contract_verified\":" << (!mismatches ? "true" : "false") << ",\"lives\":" << records.size()
                << ",\"state_checks\":" << state_checks << ",\"work_checks\":" << work_checks << ",\"mismatches\":" << mismatches
                << ",\"source_builds_retimed\":" << ignored_builds << ",\"source_clears_derived\":" << independent_clears
                << ",\"seconds\":" << seconds << ",\"financial_or_schedule_equivalence\":false}\n";
        std::cout << records.size() << " lives; " << mismatches << " biological mismatches; " << seconds << " seconds\n";
        return mismatches ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
