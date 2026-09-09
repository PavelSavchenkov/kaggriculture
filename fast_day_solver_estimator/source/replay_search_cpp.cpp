#include "query_policy.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

double cpu_seconds() {
    timespec time{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time)) throw std::runtime_error("thread CPU clock failed");
    return double(time.tv_sec) + time.tv_nsec * 1e-9;
}

struct Input {
    day_solver::DayProblem problem;
    uint64_t allowed = 0;
    unsigned tie = 0;
    std::array<unsigned, 41> query_tie{};
    std::array<bool, 41> success{};
    std::array<double, 41> seconds{};
    std::array<std::string, 41> ids{};
};

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: replay_search_cpp manifest output_directory");
        const std::filesystem::path directory = argv[2];
        if (std::filesystem::exists(directory)) throw std::runtime_error("output exists");
        std::filesystem::create_directories(directory);
        std::ifstream stream(argv[1]);
        std::ofstream output(directory / "CALLS.csv"), tests(directory / "TESTS.csv");
        if (!stream || !output || !tests) throw std::runtime_error("cannot open files");
        output << "test,step,query,success,best_workers,backend_cpu_seconds,decision_cpu_seconds,elapsed_cpu_seconds,forecasts,deferred\n" << std::setprecision(17);
        tests << "test,proposals,scoring_cpu_seconds,scoring_wall_seconds,decision_cpu_seconds,backend_cpu_seconds,total_cpu_seconds,calls,forecasts,deferral_cpu_seconds\n" << std::setprecision(17);
        std::string test; int strategy, model, use_cpu, use_logistic, count;
        while (stream >> test >> strategy >> model >> use_cpu >> use_logistic >> count) {
            if (count < 1 || count > 1000 || strategy < 0 || strategy > 5 || model < 0 || model > 2)
                throw std::runtime_error("invalid test dimensions");
            std::vector<Input> inputs(count);
            for (auto& input : inputs) {
                std::string path; int queries;
                stream >> path >> input.tie >> queries;
                if (queries < 0 || queries > 40) throw std::runtime_error("bad query count");
                input.problem = day_solver::load_problem_json(path);
                for (int i = 0; i < queries; ++i) {
                    int workers, success; unsigned tie; double seconds; std::string id;
                    stream >> workers >> tie >> success >> seconds >> id;
                    if (workers < 1 || workers > 40 || seconds < 0 || success < 0 || success > 1)
                        throw std::runtime_error("bad query record");
                    const uint64_t bit = uint64_t{1} << workers;
                    if (input.allowed & bit) throw std::runtime_error("duplicate workforce query");
                    input.allowed |= bit; input.query_tie[workers] = tie;
                    input.success[workers] = success; input.seconds[workers] = seconds; input.ids[workers] = id;
                }
            }
            if (!stream) throw std::runtime_error("truncated input");
            const auto wall_begin = std::chrono::steady_clock::now();
            const double begin = cpu_seconds();
            std::vector<labor::ScoredProposal> values;
            values.reserve(count);
            for (const auto& input : inputs) {
                auto proposal = labor::score_proposal(input.problem, 24, input.allowed, input.tie, labor::CostModel(model));
                proposal.query_tie = input.query_tie;
                values.push_back(std::move(proposal));
            }
            labor::QueryPolicy policy(std::move(values), labor::QueryOrder(strategy), use_cpu, use_logistic);
            const double scoring = cpu_seconds() - begin;
            const double scoring_wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall_begin).count();
            double elapsed = scoring, decision_total = 0, backend_total = 0, deferral = -1;
            int step = 0;
            while (true) {
                const double start = cpu_seconds();
                const auto query = policy.next();
                const double decision = cpu_seconds() - start;
                decision_total += decision; elapsed += decision;
                if (query.proposal < 0) break;
                // Only now consult the recorded outcome of the chosen call.
                const auto& input = inputs[query.proposal];
                const double backend = input.seconds[query.workers];
                const bool success = input.success[query.workers];
                elapsed += backend; backend_total += backend;
                policy.observe(query, success);
                if (policy.deferred && deferral < 0) deferral = elapsed;
                output << test << ',' << step++ << ',' << input.ids[query.workers] << ',' << success << ',' << policy.best_workers
                       << ',' << backend << ',' << decision << ',' << elapsed << ',' << policy.forecasts << ',' << policy.deferred << '\n';
            }
            tests << test << ',' << count << ',' << scoring << ',' << scoring_wall << ',' << decision_total << ',' << backend_total
                  << ',' << elapsed << ',' << step << ',' << policy.forecasts << ',' << deferral << '\n';
        }
        if (!stream.eof()) throw std::runtime_error("malformed input");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
