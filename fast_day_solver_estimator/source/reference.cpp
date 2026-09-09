#include <day_solver/io.hpp>
#include <day_solver/scheduler.hpp>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#ifndef LABOR_ACTIVE_HOURS
#define LABOR_ACTIVE_HOURS 24
#endif

void save(const std::array<kag::Action, 24>& actions, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot save schedule");
    for (const auto& a : actions) {
        out << a.n_units << ' ' << a.n_orders;
        for (int u = 0; u < a.n_units; ++u) out << ' ' << +a.units[u].op << ' ' << +a.units[u].arg << ' ' << a.units[u].n;
        for (int s = 0; s < a.n_orders; ++s) out << ' ' << +a.orders[s].op << ' ' << +a.orders[s].item << ' ' << a.orders[s].n;
        out << '\n';
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: reference manifest.txt output_folder seconds");
        std::ifstream input(argv[1]); const std::filesystem::path output(argv[2]);
        const double seconds = std::stod(argv[3]);
        if (!input || std::filesystem::exists(output) || seconds <= 0) throw std::runtime_error("invalid arguments/output exists");
        std::filesystem::create_directories(output);
        std::ofstream report(output / "results.jsonl");
        std::string id, path;
        while (input >> id >> path) {
            const auto problem = day_solver::load_problem_json(path);
            if constexpr (LABOR_ACTIVE_HOURS == 23)
                for (const auto& event : problem.market_plan)
                    if (event.hour >= 23) throw std::runtime_error("terminal input has a virtual-phase purchase");
            day_scheduler::Options options; options.seconds = seconds; options.fallback_workers = 1;
            const auto started = std::chrono::steady_clock::now(); const auto cpu_start = std::clock();
            const auto result = day_scheduler::solve(problem, options);
            auto schedule = result.schedule;
            uint64_t schedule_hash = 0;
            if (schedule) {
                if constexpr (LABOR_ACTIVE_HOURS == 23) {
                    // Remove the virtual phase entirely. Harmless final travel
                    // or transfers may disappear; required field work may not.
                    auto& last = (*schedule)[23]; const int units = last.n_units;
                    last = kag::Action{}; last.n_units = units; last.finalize();
                }
                const auto replay = day_solver::replay_schedule(problem, *schedule);
                const bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
                if (!valid) {
                    if constexpr (LABOR_ACTIVE_HOURS == 24) throw std::runtime_error("root result failed independent strict replay");
                    schedule.reset();
                } else schedule_hash = replay.candidate.replay.schedule_hash;
            }
            const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            const double cpu = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;
            if (schedule) save(*schedule, output / (id + ".actions.txt"));
            report << std::setprecision(12) << "{\"id\":\"" << id << "\",\"workers\":" << problem.worker_count
                << ",\"status\":\"" << (schedule ? "FEASIBLE" : "UNKNOWN") << "\",\"budget_seconds\":" << seconds
                << ",\"wall_seconds\":" << wall << ",\"cpu_seconds\":" << cpu
                << ",\"active_hours\":" << LABOR_ACTIVE_HOURS << ",\"virtual_phase_removal_failed\":" << (bool(result.schedule) && !schedule ? "true" : "false")
                << ",\"schedule_hash\":" << schedule_hash << "}\n";
            report.flush();
        }
        if (!input.eof()) throw std::runtime_error("bad manifest");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
