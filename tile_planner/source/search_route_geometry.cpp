#include "route_geometry.hpp"
#include <iomanip>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3 || fs::exists(argv[2])) throw std::runtime_error("usage: search_route_geometry course new_output");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]); const fs::path output(argv[2]); fs::create_directories(output);
        const RouteGeometry geometry(course); Evaluator evaluate(course);
        std::vector<std::pair<int, int>> moves;
        for (int a = 0; a < 100; ++a) if (!course.fixed[a])
            for (int b = a + 1; b < 100; ++b) if (!course.fixed[b] && quadrant(a) == quadrant(b) && (course.actions[a] || course.actions[b])) moves.emplace_back(a, b);
        struct Proposal { std::string name; Layout layout; GeometryScore route; };
        const auto baseline = geometry(identity());
        std::vector<Proposal> singles, selected{{"source", identity(), baseline}};
        int queries = 0, feasible = 0;
        for (const auto& [a, b] : moves) {
            auto layout = identity(); std::swap(layout[a], layout[b]); const auto score = geometry(layout); ++queries;
            if (!score.excess) { ++feasible; singles.push_back({"swap_" + std::to_string(a) + "_" + std::to_string(b), layout, score}); }
        }
        auto add = [&](const Proposal& p) {
            if (std::none_of(selected.begin(), selected.end(), [&](const auto& old) { return old.layout == p.layout; })) selected.push_back(p);
        };
        for (bool weighted : {false, true}) {
            auto rank = [&](const Proposal& a, const Proposal& b) {
                return std::pair{weighted ? a.route.weighted_distance : a.route.distance, a.name} < std::pair{weighted ? b.route.weighted_distance : b.route.distance, b.name};
            };
            std::sort(singles.begin(), singles.end(), rank);
            for (size_t i = 0; i < std::min<size_t>(8, singles.size()); ++i) add(singles[i]);
            for (bool blocks : {false, true}) {
                Proposal current{"source", identity(), baseline};
                for (int pass = 0; pass < 12; ++pass) {
                    Proposal best = current;
                    for (const auto& [a, b] : moves) {
                        if (blocks && quadrant(a) != pass % 4) continue;
                        auto layout = current.layout; std::swap(layout[a], layout[b]); const auto score = geometry(layout); ++queries;
                        const auto value = weighted ? score.weighted_distance : score.distance;
                        const auto best_value = weighted ? best.route.weighted_distance : best.route.distance;
                        if (!score.excess && value < best_value) best = {"descent", layout, score};
                    }
                    if (best.layout == current.layout && !blocks) break;
                    current = best;
                }
                current.name = std::string(weighted ? "peak" : "travel") + (blocks ? "_blocks" : "_joint"); add(current);
            }
        }
        std::ofstream report(output / "candidates.csv");
        report << "name,changed_cells,route_distance,weighted_distance,excess,predicted_cost,retained_days,bill\n" << std::setprecision(12);
        int complete = 0;
        for (const auto& proposal : selected) {
            const auto folder = output / proposal.name; fs::create_directories(folder);
            std::ofstream layout_file(folder / "layout.txt"); for (int cell : proposal.layout) layout_file << cell << ' '; layout_file << '\n';
            std::ofstream days(folder / "days.csv"); days << "day,retained,workers\n";
            int retained = 0; int64_t bill = 0;
            for (int d = 0; d < 30; ++d) {
                const auto certificate = certify_day(course, d, proposal.layout, 0, 0);
                days << d << ',' << bool(certificate.schedule) << ',' << certificate.workers << '\n';
                if (!certificate.schedule) continue;
                ++retained; bill += labor::hire_cost(certificate.workers);
                const auto day_folder = folder / day_name(d); fs::create_directories(day_folder);
                day_solver::save_problem_json(*certificate.problem, day_folder / "problem.json");
                labor::offline::save_actions(*certificate.schedule, (day_folder / "physical.actions.txt").string());
                labor::offline::save_actions(executable(course.days[d], *certificate.problem, *certificate.schedule), (day_folder / "executable.actions.txt").string());
            }
            complete += retained == 30;
            int changed = 0; for (int i = 0; i < 100; ++i) changed += proposal.layout[i] != i;
            report << proposal.name << ',' << changed << ',' << proposal.route.distance << ',' << proposal.route.weighted_distance << ',' << proposal.route.excess
                   << ',' << evaluate(proposal.layout).cost << ',' << retained << ',' << bill << '\n';
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"edges\":" << geometry.size() << ",\"geometry_queries\":" << queries << ",\"single_swaps\":" << moves.size()
                << ",\"window_feasible_swaps\":" << feasible << ",\"selected\":" << selected.size() << ",\"complete_physical_courses\":" << complete
                << ",\"solver_queries\":0,\"seconds\":" << seconds << "}\n";
        std::cout << complete << '/' << selected.size() << " complete physical courses; " << seconds << " seconds\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
