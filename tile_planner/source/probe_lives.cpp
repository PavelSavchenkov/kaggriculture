#include "life_search.hpp"
#include <map>
#include <unordered_set>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 4) throw std::runtime_error("usage: probe_lives incumbent_course new_output seed");
        const fs::path source(argv[1]), output(argv[2]);
        const auto began = std::chrono::steady_clock::now();
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        const auto program = load_placement_program((source / "INPUT.plan").string()); const auto& lives = program.lives;
        const auto original = read_life_assignment(source / "assignment.txt", lives.size());
        std::array<DayProblem, 30> reference;
        std::array<Day, 30> finance_days;
        const bool fixed_finance = fs::exists(source / "finance_source.txt");
        std::array<int, 30> ranked_days; std::iota(ranked_days.begin(), ranked_days.end(), 0);
        for (int d = 0; d < 30; ++d) {
            reference[d] = day_solver::load_problem_json(source / day_name(d) / "problem.json");
            if (fixed_finance) {
                finance_days[d].problem = reference[d];
                finance_days[d].executable = labor::offline::read_actions((source / day_name(d) / "executable.actions.txt").string());
            }
        }
        std::stable_sort(ranked_days.begin(), ranked_days.end(), [&](int a, int b) { return reference[a].worker_count > reference[b].worker_count; });
        const std::array<int, 2> peaks{ranked_days[0], ranked_days[1]};
        LifeEvaluator evaluate(lives, program.land, true, true, false, fixed_finance ? &finance_days : nullptr); LifeEquivalence equivalence(lives);
        struct Probe { std::string family, description; LifeAssignment assignment; Score score; uint32_t mask; };
        std::vector<Probe> probes;
        std::unordered_set<LifeAssignment, AssignmentHash> seen;
        auto add = [&](std::string family, std::string description, LifeAssignment assignment) {
            if (!legal_lives(lives, assignment)) return;
            equivalence.normalize(assignment);
            if (!seen.insert(assignment).second) return;
            uint32_t mask = 0;
            // Compare physical work/state, rather than interchangeable life IDs.
            auto before = original; equivalence.normalize(before);
            for (size_t i = 0; i < lives.size(); ++i) if (assignment[i] != before[i])
                for (int d = lives[i].spec.begin_day; d <= std::min(29, lives[i].release_day); ++d) mask |= uint32_t{1} << d;
            probes.push_back({std::move(family), std::move(description), assignment, evaluate(assignment), mask});
        };
        add("control", "incumbent", original);
        std::vector<int> crop_dates, wheat_dates;
        for (const auto& life : lives) {
            if (kag::is_crop(life.spec.item)) crop_dates.push_back(life.spec.begin_day);
            if (life.spec.item == kag::WHEAT) wheat_dates.push_back(life.spec.begin_day);
        }
        for (auto* dates : {&crop_dates, &wheat_dates}) { std::sort(dates->begin(), dates->end()); dates->erase(std::unique(dates->begin(), dates->end()), dates->end()); }
        for (int begin : crop_dates) for (int from = 0; from < 100; ++from) {
            std::vector<int> chain;
            for (size_t i = 0; i < lives.size(); ++i)
                if (original[i] == from && kag::is_crop(lives[i].spec.item) && lives[i].spec.begin_day >= begin) chain.push_back(i);
            if (chain.empty()) continue;
            for (int to = 0; to < 100; ++to) if (to != from) {
                auto a = original; for (int id : chain) a[id] = to;
                add("crop_chain", std::to_string(begin) + ":" + std::to_string(from) + ":" + std::to_string(to), a);
            }
        }
        for (size_t i = 0; i < lives.size(); ++i) if (kag::is_animal(lives[i].spec.item)) {
            for (size_t j = i + 1; j < lives.size(); ++j) if (kag::is_animal(lives[j].spec.item) && lives[i].spec.item != lives[j].spec.item) {
                auto a = original; std::swap(a[i], a[j]);
                add("animal_swap", std::to_string(i) + ":" + std::to_string(j), a);
            }
            for (int cell = 0; cell < 100; ++cell) {
                auto a = original; a[i] = cell;
                add("animal_move", std::to_string(i) + ":" + std::to_string(cell), a);
            }
        }
        // Keep up to three late wheat cohorts aligned. Quadrants remain jointly
        // scored and scheduled; only the proposal is split into blocks.
        if (wheat_dates.size() > 3) wheat_dates.erase(wheat_dates.begin(), wheat_dates.end() - 3);
        std::vector<std::vector<int>> cohort(wheat_dates.size());
        std::vector<bool> late(lives.size());
        for (size_t i = 0; i < lives.size(); ++i) if (lives[i].spec.item == kag::WHEAT) {
            const auto found = std::find(wheat_dates.begin(), wheat_dates.end(), lives[i].spec.begin_day);
            if (found != wheat_dates.end()) { cohort[found - wheat_dates.begin()].push_back(i); late[i] = true; }
        }
        const bool aligned = !cohort.empty() && std::all_of(cohort.begin(), cohort.end(), [&](const auto& ids) { return ids.size() == cohort[0].size(); });
        std::array<std::vector<int>, 4> available;
        for (int cell = 0; aligned && cell < 100; ++cell) {
            bool free = true;
            for (size_t i = 0; i < lives.size(); ++i) if (original[i] == cell && !late[i]) free &= lives[i].release_day <= wheat_dates[0];
            if (free && lives[cohort[0][0]].spec.allowed[cell]) available[quadrant(cell)].push_back(cell);
        }
        for (auto& cells : available) std::sort(cells.begin(), cells.end(), [](int a, int b) { return std::pair{labor::shed_distance(a), a} < std::pair{labor::shed_distance(b), b}; });
        const int count = aligned ? cohort[0].size() : 0;
        for (int a = 0; aligned && a <= int(available[0].size()); ++a) for (int b = 0; b <= int(available[1].size()); ++b) {
            const int first_c = std::max(0, count - a - b - int(available[3].size()));
            const int last_c = std::min(int(available[2].size()), count - a - b);
            for (int c = first_c; c <= last_c; ++c) {
                const int fourth = count - a - b - c;
                auto assignment = original; std::vector<int> cells;
                for (int q = 0; q < 4; ++q) cells.insert(cells.end(), available[q].begin(), available[q].begin() + std::array{a, b, c, fourth}[q]);
                for (const auto& ids : cohort) for (int i = 0; i < count; ++i) assignment[ids[i]] = cells[i];
                add("quadrant_counts", std::to_string(a) + ":" + std::to_string(b) + ":" + std::to_string(c) + ":" + std::to_string(fourth), assignment);
            }
        }
        std::mt19937_64 rng(std::stoull(argv[3]));
        std::map<std::string, std::vector<int>> families;
        for (int i = 0; i < int(probes.size()); ++i) families[probes[i].family].push_back(i);
        std::unordered_set<int> selected{0};
        for (auto& [family, ids] : families) {
            std::sort(ids.begin(), ids.end(), [&](int a, int b) { return probes[a].score.day_cost[peaks[0]] + probes[a].score.day_cost[peaks[1]] < probes[b].score.day_cost[peaks[0]] + probes[b].score.day_cost[peaks[1]]; });
            for (int i = 0; i < std::min(3, int(ids.size())); ++i) selected.insert(ids[i]);
            selected.insert(ids.back());
            std::shuffle(ids.begin(), ids.end(), rng);
            for (int i = 0; i < std::min(4, int(ids.size())); ++i) selected.insert(ids[i]);
        }
        std::ofstream table(output / "probes.csv"); table << "id,family,description,predicted_bill,peak0_cost,peak1_cost,affected_mask,selected\n";
        for (int i = 0; i < int(probes.size()); ++i) {
            const auto& p = probes[i];
            table << i << ',' << p.family << ',' << p.description << ',' << p.score.cost << ',' << p.score.day_cost[peaks[0]] << ',' << p.score.day_cost[peaks[1]] << ',' << p.mask << ',' << selected.contains(i) << '\n';
            if (!selected.contains(i)) continue;
            const auto folder = output / std::to_string(i); fs::create_directories(folder);
            save_life_assignment(lives, p.assignment, folder); save_placement_program(program, folder / "INPUT.plan");
            kag::Config config; config.weed_chance = 0; auto farm = kag::Sim(config).st.farms[0];
            for (int d = 0; d < 30; ++d) {
                // Use the incumbent's actual stock, so these point probes hold
                // finance quantities fixed while changing physical placement.
                std::copy(reference[d].start.shed.begin(), reference[d].start.shed.end(), farm.shed);
                std::copy(reference[d].start.seeds.begin(), reference[d].start.seeds.end(), farm.seeds);
                for (int cell = 0; cell < 100; ++cell) {
                    auto& tile = farm.tiles[cell / 10][cell % 10]; const auto& state = reference[d].start.managed_tiles[cell].state;
                    using K = day_solver::ManagedTileKind;
                    if ((tile.kind == kag::T_EMPTY || tile.kind == kag::T_WEED) && (state.kind == K::EMPTY || state.kind == K::WEED))
                        tile = physical_tile(state, d);
                }
                auto contract = compile_life_day(lives, p.assignment, d, farm, program.land[d], false);
                if (fixed_finance) apply_fixed_finance(contract, finance_days[d], d);
                if (d == peaks[0] || d == peaks[1]) {
                    const auto point = folder / day_name(d); fs::create_directories(point);
                    day_solver::save_problem_json(contract.day.problem, point / "problem.json");
                    labor::offline::save_actions(contract.day.executable, point / "executable.actions.txt");
                }
                for (const auto& end : contract.day.problem.required_end_tiles) farm.tiles[end.tile / 10][end.tile % 10] = physical_tile(*end.exact_state, d + 1);
                farm.n_quadrants += program.land[d];
            }
        }
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"probes\":" << probes.size() << ",\"selected\":" << selected.size() << ",\"peak_days\":[" << peaks[0] << ',' << peaks[1]
                << "],\"day_queries\":" << evaluate.day_queries << ",\"day_cache_hits\":" << evaluate.day_hits << ",\"day_cache_evictions\":" << evaluate.day_evictions
                << ",\"fixed_finance\":" << (fixed_finance ? "true" : "false")
                << ",\"point_scope\":\"projected candidate biology with source stocks and weeds on empty cells; full-game verification still required\",\"seconds\":"
                << std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count() << "}\n";
        std::cout << probes.size() << " legal distinct probes; " << selected.size() << " selected for exact point evaluation\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
