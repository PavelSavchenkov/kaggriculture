#include "long_retry_policy.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>

double cpu_seconds() {
    timespec t{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t)) throw std::runtime_error("thread CPU clock failed");
    return t.tv_sec + t.tv_nsec * 1e-9;
}

struct Input {
    std::string id;
    int prior;
    double threshold, failed, successful;
    std::vector<labor::RetryOption> options;
    std::array<bool, 41> success{};
    std::array<double, 41> cpu{};
};

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: replay_long_retry_cpp input output repeats");
        const int repeats = std::stoi(argv[3]);
        if (repeats < 1) throw std::runtime_error("invalid repeats");
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open input");
        std::vector<Input> inputs;
        Input row; int n;
        while (input >> row.id >> row.prior >> row.threshold >> row.failed >> row.successful >> n) {
            if (n < 0 || n > 39) throw std::runtime_error("invalid option count");
            row.options.clear();
            for (int i = 0; i < n; ++i) {
                int workers, success; double p, cpu;
                if (!(input >> workers >> p >> success >> cpu) || workers < 1 || workers > 40 ||
                    (success != 0 && success != 1) || cpu < 0 || !std::isfinite(cpu))
                    throw std::runtime_error("invalid recorded option");
                row.options.push_back({workers, p}); row.success[workers] = success; row.cpu[workers] = cpu;
            }
            inputs.push_back(row);
        }
        if (!input.eof()) throw std::runtime_error("incomplete input");
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot open output");
        output << "id,pass,final_workers,final_bill,backend_cpu,policy_cpu_us,choices\n" << std::setprecision(17);
        std::vector<int> order(inputs.size()); std::iota(order.begin(), order.end(), 0);
        std::mt19937 random(909925);
        for (int pass = 0; pass < repeats; ++pass) {
            std::shuffle(order.begin(), order.end(), random);
            for (int index : order) {
                const auto& r = inputs[index];
                const double begin = cpu_seconds();
                labor::LongRetryPolicy policy(r.prior, r.options, r.threshold, r.failed, r.successful);
                std::array<int, 39> choices{}; int calls = 0; double backend = 0;
                for (;;) {
                    const auto choice = policy.next();
                    if (!choice.workers) break;
                    choices[calls++] = choice.workers;
                    backend += r.cpu[choice.workers];
                    policy.observe(choice.workers, r.success[choice.workers]);
                }
                const double elapsed = (cpu_seconds() - begin) * 1e6;
                output << r.id << ',' << pass << ',' << policy.best_workers << ',' << labor::hire_cost(policy.best_workers)
                       << ',' << backend << ',' << elapsed << ',';
                for (int i = 0; i < calls; ++i) output << (i ? ":" : "") << choices[i];
                output << '\n';
            }
        }
        std::cout << inputs.size() * repeats << " retry decision replays\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
