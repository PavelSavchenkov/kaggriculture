#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using Schedule = std::array<kag::Action, 24>;

Schedule read_actions(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read actions: " + path);
    Schedule actions;
    for (auto& action : actions) {
        input >> action.n_units >> action.n_orders;
        if (!input || action.n_units < 1 || action.n_units > kag::MAX_UNITS || action.n_orders < 0 || action.n_orders > 10)
            throw std::runtime_error("invalid action dimensions");
        for (int u = 0; u < action.n_units; ++u) {
            int op, arg; input >> op >> arg >> action.units[u].n;
            if (op < 0 || op > kag::OP_CARE || arg < 0 || arg >= kag::N_ITEMS) throw std::runtime_error("invalid unit action");
            action.units[u].op = op; action.units[u].arg = arg;
        }
        for (int s = 0; s < action.n_orders; ++s) {
            int op, item; input >> op >> item >> action.orders[s].n;
            if (op < 0 || op > kag::M_SELL || item < 0 || item >= kag::N_ITEMS) throw std::runtime_error("invalid order");
            action.orders[s].op = op; action.orders[s].item = item;
        }
        action.finalize();
    }
    std::string extra;
    if (!input || input >> extra) throw std::runtime_error("invalid action file length");
    return actions;
}

void physical_orders(const day_solver::DayProblem& problem, Schedule& actions) {
    for (auto& action : actions) {
        action.n_orders = 0;
        std::fill(std::begin(action.orders), std::end(action.orders), kag::Order{});
    }
    for (const auto& event : problem.market_plan) {
        auto& action = actions[event.hour];
        action.n_orders = std::max(action.n_orders, int(event.order_index) + 1);
        action.orders[event.order_index] = {event.market_op, uint8_t(std::max(0, int(event.item))), event.quantity};
    }
    for (auto& action : actions) action.finalize();
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: labor_tool features|bounds|audit manifest.txt output.csv");
        const std::string mode = argv[1];
        std::ifstream input(argv[2]); std::ofstream output(argv[3]);
        std::ofstream errors(std::string(argv[3]) + ".errors.txt");
        if (!input || !output) throw std::runtime_error("cannot open manifest/output");
        if (mode == "features") {
            output << "id"; for (const auto& name : labor::feature_names()) output << ',' << name;
            output << ",extraction_us\n";
        } else if (mode == "audit") output << "id,strict,requirements,invariants,errors,hires,travel,actions,hash\n";
        else if (mode == "bounds") output << "id,lower_bound,deadline_missing_quantity\n";
        else throw std::runtime_error("unknown mode");
        std::string id, path, witness; int count = 0;
        while (input >> id >> path) {
            const auto problem = day_solver::load_problem_json(path);
            if (mode == "features") {
                const auto start = std::chrono::steady_clock::now();
                const auto f = labor::extract(problem);
                const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
                output << id << std::setprecision(9);
                for (float v : f) output << ',' << v;
                output << ',' << us << '\n';
            } else if (mode == "bounds") {
                const auto f = labor::extract(problem);
                const int bound = labor::workforce_lower_bound(problem, f, labor::earliest_menu(problem));
                output << id << ',' << bound << ',' << f[labor::deadline_missing_quantity] << '\n';
            } else {
                if (!(input >> witness)) throw std::runtime_error("missing witness path");
                auto actions = read_actions(witness);
                physical_orders(problem, actions);
                const auto replay = day_solver::replay_schedule(problem, actions);
                for (const auto& error : replay.errors) errors << id << '\t' << error << '\n';
                output << id << ',' << replay.candidate.replay.strict_valid << ',' << replay.requirements_satisfied
                    << ',' << replay.invariants_satisfied << ',' << replay.errors.size() << ',' << replay.candidate.costs.hires
                    << ',' << replay.candidate.costs.travel_actions << ',' << replay.candidate.costs.unit_actions
                    << ',' << replay.candidate.replay.schedule_hash << '\n';
            }
            if (++count % 100 == 0) output.flush();
        }
        if (!input.eof()) throw std::runtime_error("invalid manifest");
        std::cout << "completed " << count << ' ' << mode << " records\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 2;
    }
}
