#include "query_policy.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: check_query_policy manifest output");
        std::ifstream input(argv[1]); std::ofstream output(argv[2]);
        if (!input || !output) throw std::runtime_error("cannot open input/output");
        output << "test,step,query,success,best_workers,decision_us,forecasts\n" << std::setprecision(17);
        std::string id; int strategy, use_cpu, use_logistic, count, tests = 0, calls = 0;
        while (input >> id >> strategy >> use_cpu >> use_logistic >> count) {
            if (count < 1 || count > 1000 || strategy < 0 || strategy > 5) throw std::runtime_error("bad test dimensions");
            std::vector<labor::ScoredProposal> values(count);
            std::vector<std::array<bool, 41>> outcomes(count);
            std::vector<std::array<std::string, 41>> ids(count);
            for (int i = 0; i < count; ++i) {
                auto& p = values[i]; std::string case_id; int queries;
                input >> case_id >> p.lower >> p.screened >> p.workers >> p.tie >> queries;
                if (queries < 0 || queries > 40) throw std::runtime_error("bad query count");
                for (int q = 0; q < queries; ++q) {
                    int workers, success; double probability, cpu; unsigned tie; std::string name;
                    input >> workers >> probability >> cpu >> tie >> success >> name;
                    if (workers < 1 || workers > 40) throw std::runtime_error("bad query workforce");
                    p.allowed |= uint64_t{1} << workers; p.probability[workers] = probability; p.cpu[workers] = cpu;
                    p.query_tie[workers] = tie; outcomes[i][workers] = success; ids[i][workers] = name;
                }
                p.forecasted = p.allowed;
            }
            if (!input) throw std::runtime_error("truncated test input");
            labor::QueryPolicy policy(std::move(values), labor::QueryOrder(strategy), use_cpu, use_logistic);
            int step = 0;
            while (true) {
                const auto start = std::chrono::steady_clock::now();
                const auto query = policy.next();
                const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
                if (query.proposal < 0) break;
                const bool success = outcomes[query.proposal][query.workers];
                policy.observe(query, success);
                output << id << ',' << step++ << ',' << ids[query.proposal][query.workers] << ',' << success << ',' << policy.best_workers << ',' << us << ',' << policy.forecasts << '\n';
                ++calls;
            }
            ++tests;
        }
        if (!input.eof()) throw std::runtime_error("malformed test manifest");
        std::cout << tests << " C++ query-policy cases, " << calls << " decisions\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
