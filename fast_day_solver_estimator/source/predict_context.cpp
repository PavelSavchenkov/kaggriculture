#include "context_model.hpp"
#include <day_solver/io.hpp>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>

double thread_seconds() {
    timespec time{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time)) throw std::runtime_error("thread CPU clock failed");
    return time.tv_sec + time.tv_nsec * 1e-9;
}

struct Input {
    std::string id;
    day_solver::DayProblem problem;
    int hours, fixed_count, optional_count;
    std::array<std::pair<int, int>, 39> fixed{}, optional{};
};
struct Prediction {
    int input, pass, workers, lower;
    double boost, logistic, cpu, feature_us, query_us;
};

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: predict_context manifest output repeats");
        const int repeats = std::stoi(argv[3]);
        if (repeats < 1) throw std::runtime_error("invalid repeats");
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open manifest");
        std::vector<Input> inputs;
        Input row; std::string path;
        while (input >> row.id >> path >> row.hours >> row.fixed_count >> row.optional_count) {
            if (row.fixed_count < 0 || row.optional_count < 0 || row.fixed_count + row.optional_count > 39)
                throw std::runtime_error("invalid menu sizes");
            for (int i = 0; i < row.fixed_count; ++i)
                if (!(input >> row.fixed[i].first >> row.fixed[i].second)) throw std::runtime_error("incomplete fixed menu");
            for (int i = 0; i < row.optional_count; ++i)
                if (!(input >> row.optional[i].first >> row.optional[i].second)) throw std::runtime_error("incomplete optional menu");
            row.problem = day_solver::load_problem_json(path); inputs.push_back(std::move(row));
        }
        if (!input.eof()) throw std::runtime_error("malformed context manifest");
        std::vector<int> order(inputs.size()); std::iota(order.begin(), order.end(), 0);
        std::mt19937 random(909); std::vector<Prediction> predictions;
        predictions.reserve(inputs.size() * repeats * 40);
        for (int pass = 0; pass < repeats; ++pass) {
            std::shuffle(order.begin(), order.end(), random);
            for (int index : order) {
                const auto& r = inputs[index];
                const double begin = thread_seconds();
                const auto menu = labor::fixed_planning_menu(r.problem, r.hours,
                    std::span(r.fixed.data(), r.fixed_count), std::span(r.optional.data(), r.optional_count));
                const auto f = labor::extract_context(r.problem, menu);
                const double feature_us = (thread_seconds() - begin) * 1e6;
                for (int workers = menu.minimum_workers; workers <= menu.size + 1; ++workers) {
                    const double start = thread_seconds();
                    const auto q = labor::context_query_features(f, workers);
                    const double boost = labor::context_model::boost(q), logistic = labor::context_model::logistic(q), cpu = labor::context_model::cpu(q);
                    predictions.push_back({index, pass, workers, int(f[labor::planning_lower]), boost, logistic, cpu,
                                           feature_us, (thread_seconds() - start) * 1e6});
                }
            }
        }
        std::ofstream output(argv[2]); if (!output) throw std::runtime_error("cannot open output");
        output << "id,pass,workers,lower_bound,boost,logistic,cpu,feature_cpu_us,query_cpu_us\n" << std::setprecision(17);
        for (const auto& p : predictions)
            output << inputs[p.input].id << ',' << p.pass << ',' << p.workers << ',' << p.lower << ',' << p.boost << ',' << p.logistic << ',' << p.cpu << ',' << p.feature_us << ',' << p.query_us << '\n';
        std::cout << predictions.size() << " context-query C++ predictions\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
