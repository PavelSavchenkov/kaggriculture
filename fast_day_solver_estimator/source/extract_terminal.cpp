#include "replay_trace.hpp"

void extract_terminal(const Trace& trace, int seat, const fs::path& output) {
    Sim sim(trace.config); std::optional<RecordedDay> day;
    for (const auto& turn : trace.turns) {
        const auto before = sim; sim.step(turn.a[0], turn.a[1]);
        if (before.st.day != 29) continue;
        auto own_before = before, own_after = sim;
        Action pair[2] = {turn.a[seat], turn.a[seat ^ 1]};
        if (seat) { std::swap(own_before.st.farms[0], own_before.st.farms[1]); std::swap(own_after.st.farms[0], own_after.st.farms[1]); }
        if (before.st.hour == 0) day.emplace(own_before);
        append_contract(*day, own_before, own_after, pair);
    }
    if (!day || sim.st.day != 29 || sim.st.hour != 23 || !sim.st.done) throw std::runtime_error("unexpected terminal boundary");
    const auto terminal_hash = sim.parity_hash();
    auto before = sim;
    if (seat) std::swap(before.st.farms[0], before.st.farms[1]);
    std::array<int64_t, N_ITEMS> actual_shed{}, actual_carried{};
    std::copy_n(before.st.farms[0].shed, N_ITEMS, actual_shed.begin());
    for (int u = 0; u < before.st.farms[0].n_units; ++u) for (int item = 0; item < N_ITEMS; ++item)
        actual_carried[item] += before.st.farms[0].inv[u][item];
    // A bookkeeping phase supplies the public v3 dawn-to-dawn endpoint. It is
    // forbidden to do any work or trade in this virtual phase. Its automatic
    // transfer represents remaining total goods, not a real terminal deposit.
    before.st.done = false; before.cfg.weed_chance = 0; before.cfg.shed_capacity = 30000;
    Action pass[2];
    for (int player = 0; player < 2; ++player) { pass[player].n_units = before.st.farms[player].n_units; pass[player].finalize(); }
    auto after = before; after.step(pass[0], pass[1]);
    append_contract(*day, before, after, pass);
    for (int item = 0; item < N_ITEMS; ++item)
        if (day->problem.end_shed[item] != actual_shed[item] + actual_carried[item])
            throw std::runtime_error("virtual phase changed total remaining goods");
    day_scheduler::prepare_problem(day->problem);
    const auto actions = physical(*day);
    if (actions[23].n_orders || std::any_of(actions[23].units, actions[23].units + actions[23].n_units,
        [](const auto& action) { return action.op != OP_PASS; })) throw std::runtime_error("nonempty virtual phase");
    const auto replay = replay_schedule(day->problem, actions);
    const bool valid = replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
    fs::create_directories(output);
    save_problem_json(day->problem, output / "problem.json");
    save_actions(actions, output / "physical_source.actions.txt");
    save_actions(day->own, output / "executable_source.actions.txt");
    std::ofstream report(output / "REPORT.json"), errors(output / "errors.txt");
    report << "{\"status\":\"" << (valid ? "FEASIBLE_SOURCE" : day->discarded ? "CAPACITY_LOSS_UNSUPPORTED" : "SOURCE_REJECTED")
           << "\",\"active_hours\":23,\"virtual_phase\":23,\"terminal_hash\":" << terminal_hash
           << ",\"workers\":" << day->problem.worker_count << ",\"strict\":" << replay.candidate.replay.strict_valid
           << ",\"requirements\":" << replay.requirements_satisfied << ",\"invariants\":" << replay.invariants_satisfied
           << ",\"discarded\":" << day->discarded << ",\"actual_shed\":[";
    for (int i = 0; i < N_ITEMS; ++i) report << (i ? "," : "") << actual_shed[i];
    report << "],\"actual_carried\":[";
    for (int i = 0; i < N_ITEMS; ++i) report << (i ? "," : "") << actual_carried[i];
    report << "]}\n";
    for (const auto& error : replay.errors) errors << error << '\n';
    std::cout << "720-state parity and terminal source replay " << valid << '\n';
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: extract_terminal trace.txt seat output_dir");
        const int seat = std::stoi(argv[2]); const fs::path output(argv[3]);
        if (seat < 0 || seat > 1 || fs::exists(output)) throw std::runtime_error("invalid seat or output exists");
        const auto trace = load(argv[1]); validate(trace); extract_terminal(trace, seat, output);
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
