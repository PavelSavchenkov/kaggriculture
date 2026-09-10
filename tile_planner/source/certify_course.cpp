#include "certify.hpp"
#include "timeline.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 6 || argc > 8) throw std::runtime_error("usage: certify_course course layout new_output per_day_seconds query_seconds [warm_directory [refine_day_mask]]");
        const auto began = std::chrono::steady_clock::now();
        const Course course(argv[1]);
        const auto timeline = read_timeline(argv[2]);
        if (!legal_timeline(course, timeline)) throw std::runtime_error("illegal timeline");
        const fs::path output(argv[3]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        const double budget = std::stod(argv[4]), query_budget = std::stod(argv[5]);
        std::ofstream report(output / "days.csv");
        report << "day,certified,workers,bill,queries,seconds,initial_witness\n";
        int valid = 0, queries = 0;
        int64_t bill = 0;
        double seconds = 0;
        for (int d = 0; d < 30; ++d) {
            const auto& layout = timeline[d];
            auto certificate = argc >= 7 ? warm_certificate(course, d, layout, fs::path(argv[6]) / day_name(d)) : Certificate{};
            const bool refine = argc == 8 && (std::stoull(argv[7]) & (uint64_t{1} << d));
            if (!certificate.schedule || refine) certificate = certify_day(course, d, layout, budget, query_budget, &certificate);
            const bool ok = certificate.schedule.has_value();
            const int64_t cost = ok ? labor::hire_cost(certificate.workers) : -1;
            valid += ok; queries += certificate.queries; seconds += certificate.seconds;
            if (ok) {
                bill += cost;
                const auto folder = output / day_name(d); fs::create_directories(folder);
                day_solver::save_problem_json(*certificate.problem, folder / "problem.json");
                labor::offline::save_actions(*certificate.schedule, (folder / "physical.actions.txt").string());
                labor::offline::save_actions(executable(course.days[d], *certificate.problem, *certificate.schedule),
                                            (folder / "executable.actions.txt").string());
            }
            report << d << ',' << ok << ',' << certificate.workers << ',' << cost << ',' << certificate.queries << ',' << certificate.seconds << ',' << certificate.initial_witness << '\n';
            report.flush();
            std::cout << "day " << d << " certified " << ok << " workers " << certificate.workers << " seconds " << certificate.seconds << std::endl;
        }
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"certified_days\":" << valid << ",\"complete\":" << (valid == 30 ? "true" : "false")
                << ",\"certified_partial_bill\":" << bill << ",\"queries\":" << queries << ",\"seconds\":" << seconds
                << ",\"process_seconds\":" << std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count()
                << ",\"full_engine_verified\":false}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
