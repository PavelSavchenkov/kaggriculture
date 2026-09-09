#include "actions.hpp"
#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <filesystem>
#include <iostream>

bool valid(const day_solver::ReplayResult& replay) {
    return replay.candidate.replay.strict_valid && replay.requirements_satisfied
        && replay.invariants_satisfied && replay.errors.empty();
}

int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4) throw std::runtime_error("usage: normalize_sources manifest.txt output_folder [active_hours]");
        const int hours = argc == 4 ? std::stoi(argv[3]) : 24;
        std::ifstream input(argv[1]); const std::filesystem::path output(argv[2]);
        if (!input || std::filesystem::exists(output)) throw std::runtime_error("bad arguments/output exists");
        std::filesystem::create_directories(output);
        std::ofstream report(output / "results.jsonl"), errors(output / "errors.txt");
        std::string id, path, witness;
        int count = 0, passed = 0;
        while (input >> id >> path >> witness) {
            auto problem = day_solver::load_problem_json(path);
            auto actions = labor::offline::read_actions(witness);
            labor::offline::physical_orders(problem, actions);
            const auto original = day_solver::replay_schedule(problem, actions);
            const auto menu = labor::earliest_menu(problem, hours);
            const auto features = labor::extract(problem, hours);
            const auto bound = labor::workforce_lower_bound(problem, features, menu, hours);
            const auto& last = actions[23];
            const bool terminal_valid = hours == 24 || (!last.n_orders && std::all_of(last.units, last.units + last.n_units,
                [](const auto& a) { return a.op == kag::OP_PASS; }));
            std::string status = "SOURCE_INVALID";
            uint64_t hash = 0;
            if (valid(original) && terminal_valid) {
                if (features[labor::deadline_missing_quantity] > 0)
                    throw std::runtime_error("deadline reachability contradicts a verified source: " + id);
                if (bound > problem.worker_count) throw std::runtime_error("lower bound exceeds a verified source workforce: " + id);
                std::erase_if(problem.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
                for (int u = 0; u < problem.worker_count - 1; ++u) {
                    day_solver::MarketEvent e{};
                    e.hour = menu.hours[u]; e.order_index = menu.slots[u]; e.market_op = kag::M_HIRE;
                    e.item = -1; e.quantity = 1; problem.market_plan.push_back(e);
                }
                std::sort(problem.market_plan.begin(), problem.market_plan.end(), [](const auto& a, const auto& b) {
                    return std::pair(a.hour, a.order_index) < std::pair(b.hour, b.order_index);
                });
                for (int h = 0; h < 24; ++h) {
                    int active = 1;
                    for (int u = 0; u < problem.worker_count - 1; ++u) active += menu.hours[u] < h;
                    if (active < actions[h].n_units) throw std::runtime_error("normalization delayed a worker");
                    for (int u = actions[h].n_units; u < active; ++u) actions[h].units[u] = {};
                    actions[h].n_units = active;
                }
                labor::offline::physical_orders(problem, actions);
                const auto replay = day_solver::replay_schedule(problem, actions);
                status = valid(replay) ? "FEASIBLE" : "NORMALIZATION_REJECTED";
                if (valid(replay)) {
                    hash = replay.candidate.replay.schedule_hash;
                    labor::offline::save_actions(actions, (output / (id + ".actions.txt")).string());
                    ++passed;
                }
                for (const auto& e : replay.errors) errors << id << '\t' << e << '\n';
            } else for (const auto& e : original.errors) errors << id << '\t' << e << '\n';
            report << "{\"id\":\"" << id << "\",\"workers\":" << problem.worker_count
                << ",\"lower_bound\":" << bound << ",\"active_hours\":" << hours << ",\"status\":\"" << status << "\",\"schedule_hash\":" << hash << "}\n";
            ++count;
        }
        if (!input.eof()) throw std::runtime_error("invalid manifest");
        std::cout << "strict normalized witnesses " << passed << '/' << count << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
