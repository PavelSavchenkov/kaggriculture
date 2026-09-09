#include "planning_estimator.hpp"
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
    int input, pass; bool forced; double cpu_us; labor::PlanningPrediction result;
};

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: predict_planning manifest output repeats");
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
        predictions.reserve(inputs.size() * repeats * 2);
        for (int pass = 0; pass < repeats; ++pass) {
            std::shuffle(order.begin(), order.end(), random);
            for (int index : order) {
                const auto& r = inputs[index];
                std::array<bool, 2> modes{false, true};
                if ((pass + index) % 2) std::swap(modes[0], modes[1]);
                for (bool forced : modes) {
                    const double begin = thread_seconds();
                    const auto menu = labor::fixed_planning_menu(r.problem, r.hours,
                        std::span(r.fixed.data(), r.fixed_count), std::span(r.optional.data(), r.optional_count));
                    auto prediction = labor::estimate_planning(r.problem, menu, forced);
                    predictions.push_back({index, pass, forced, (thread_seconds() - begin) * 1e6, std::move(prediction)});
                }
            }
        }
        std::ofstream output(argv[2]); if (!output) throw std::runtime_error("cannot open output");
        output << "id,pass,forced,cpu_us,rejected,direct,complete_curve,low_peak,weak_probe,lower,workers,cost,probe_workers,probe_probability,evaluations,peak,mean_workers,mean_cost,q25,q50,q75,q90,absolute_half\n" << std::setprecision(17);
        for (const auto& p : predictions) {
            const auto& r = p.result;
            output << inputs[p.input].id << ',' << p.pass << ',' << p.forced << ',' << p.cpu_us << ',' << r.analytically_rejected << ',' << r.uses_direct_cost << ',' << r.complete_curve << ',' << r.low_peak << ',' << r.weak_probe << ',' << r.lower << ',' << r.workers << ',' << r.cost << ',' << r.probe_workers << ',' << r.probe_probability << ',' << r.query_evaluations << ',' << r.curve.peak << ',' << r.curve.mean_workers << ',' << r.curve.mean_cost;
            for (int q : r.curve.quantiles) output << ',' << q;
            output << ',' << r.curve.absolute_half << '\n';
        }
        std::cout << predictions.size() << " planning predictions\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
