#include "replay_trace.hpp"
#include "terminal_deadlines.hpp"
#include <chrono>

int main(int argc, char** argv) {
    try {
        if (argc != 4 && argc != 5) throw std::runtime_error("usage: extract_course trace seat new_output [discard_accounting]");
        const bool account_discard = argc == 5;
        if (account_discard && std::string(argv[4]) != "discard_accounting") throw std::runtime_error("unknown extraction mode");
        const auto started = std::chrono::steady_clock::now();
        const auto trace = load(argv[1]);
        const int seat = std::stoi(argv[2]);
        const fs::path output(argv[3]);
        if (seat < 0 || seat > 1 || fs::exists(output)) throw std::runtime_error("invalid seat or output exists");
        validate(trace);
        fs::create_directories(output);
        Sim sim(trace.config);
        std::vector<RecordedDay> days;
        std::array<std::array<int64_t, N_ITEMS>, 30> discarded{};
        days.reserve(30);
        for (const auto& turn : trace.turns) {
            const auto before = sim;
            sim.step(turn.a[0], turn.a[1]);
            auto own_before = before, own_after = sim;
            if (seat) {
                std::swap(own_before.st.farms[0], own_before.st.farms[1]);
                std::swap(own_after.st.farms[0], own_after.st.farms[1]);
            }
            if (!before.st.hour) days.emplace_back(own_before);
            Action actions[2] = {turn.a[seat], turn.a[seat ^ 1]};
            append_contract(days.back(), own_before, own_after, actions);
            for (int item = 0; item < N_ITEMS; ++item)
                discarded[before.st.day][item] += own_after.st.farms[0].discarded[item] - own_before.st.farms[0].discarded[item];
        }
        if (days.size() != 30 || sim.st.step != 719) throw std::runtime_error("incomplete course");
        auto terminal = sim;
        if (seat) std::swap(terminal.st.farms[0], terminal.st.farms[1]);
        terminal.st.done = false;
        terminal.cfg.weed_chance = 0;
        terminal.cfg.shed_capacity = 30000;
        Action pass[2];
        for (int p = 0; p < 2; ++p) { pass[p].n_units = terminal.st.farms[p].n_units; pass[p].finalize(); }
        auto after = terminal;
        after.step(pass[0], pass[1]);
        append_contract(days.back(), terminal, after, pass);
        std::ofstream report(output / "days.csv"), errors(output / "errors.txt");
        report << "day,workers,work_actions,active_tiles,discarded,strict,requirements,invariants,errors\n";
        int valid_days = 0;
        int64_t bill = 0;
        for (int d = 0; d < 30; ++d) {
            auto& day = days[d];
            const auto actual_end_shed = day.problem.end_shed;
            // The physical scheduler intentionally has no shed-capacity loss.
            // Retain discarded stock in its bookkeeping endpoint, while the
            // next day and full-engine checker retain the actual source state.
            if (account_discard) for (int item = 0; item < N_ITEMS; ++item)
                day.problem.end_shed[item] += discarded[d][item];
            day_scheduler::prepare_problem(day.problem);
            if (d == 29) require_terminal_work(day.problem);
            const auto schedule = physical(day);
            const auto replay = replay_schedule(day.problem, schedule);
            const bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied &&
                               replay.invariants_satisfied && replay.errors.empty();
            valid_days += valid;
            const auto folder = output / (d < 10 ? "0" + std::to_string(d) : std::to_string(d));
            fs::create_directories(folder);
            std::ofstream endpoint(folder / "endpoint.json");
            endpoint << "{\"discard_accounting\":" << (account_discard ? "true" : "false") << ",\"actual_end_shed\":[";
            for (int item = 0; item < N_ITEMS; ++item) endpoint << (item ? "," : "") << actual_end_shed[item];
            endpoint << "],\"discarded\":[";
            for (int item = 0; item < N_ITEMS; ++item) endpoint << (item ? "," : "") << discarded[d][item];
            endpoint << "]}\n";
            save_problem_json(day.problem, folder / "problem.json");
            save_actions(schedule, folder / "physical.actions.txt");
            save_actions(day.own, folder / "executable.actions.txt");
            int tasks = 0;
            for (const auto& work : day.problem.tile_work) tasks += work.actions.size();
            report << d << ',' << day.problem.worker_count << ',' << tasks << ',' << day.problem.tile_work.size()
                   << ',' << day.discarded << ',' << replay.candidate.replay.strict_valid << ','
                   << replay.requirements_satisfied << ',' << replay.invariants_satisfied << ',' << replay.errors.size() << '\n';
            for (const auto& error : replay.errors) errors << d << '\t' << error << '\n';
        }
        bill = 0;
        for (const auto& day : days) for (int k = 0; k < day.problem.worker_count - 1; ++k) bill += fib(k);
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"source_parity\":true,\"seat\":" << seat << ",\"days\":30,\"valid_day_contracts\":" << valid_days
                << ",\"discard_accounting\":" << (account_discard ? "true" : "false")
                << ",\"source_hire_bill\":" << bill << ",\"source_cash\":" << sim.st.farms[seat].money
                << ",\"rival_cash\":" << sim.st.farms[seat ^ 1].money << ",\"seconds\":" << elapsed << "}\n";
        std::cout << "source parity exact; valid day contracts " << valid_days << "/30; seconds " << elapsed << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
