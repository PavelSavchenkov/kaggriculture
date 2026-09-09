#include "bounds.hpp"
#include "supply_bounds.hpp"
#include "search_model.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>

struct Prediction {
    int input, pass, workers, lower, input_missing;
    bool output_missing;
    double cost, logistic, boost, cpu, feature_us, cost_us, query_us;
};

int main(int argc, char** argv) {
    try {
        if (argc != 4 && argc != 5) throw std::runtime_error("usage: predict_search manifest output repeats [supply]");
        const bool with_supply = argc == 5;
        if (with_supply && std::string(argv[4]) != "supply") throw std::runtime_error("unknown bound mode");
        std::ifstream input(argv[1]); const int repeats = std::stoi(argv[3]);
        if (!input || repeats < 1) throw std::runtime_error("invalid arguments");
        std::vector<std::string> ids; std::vector<day_solver::DayProblem> problems;
        std::string id, path;
        while (input >> id >> path) { ids.push_back(id); problems.push_back(day_solver::load_problem_json(path)); }
        if (!input.eof()) throw std::runtime_error("malformed manifest");
        std::vector<int> order(ids.size()); std::iota(order.begin(), order.end(), 0);
        std::mt19937 random(909); std::vector<Prediction> predictions;
        predictions.reserve(ids.size() * repeats * 40);
        for (int pass = 0; pass < repeats; ++pass) {
            std::shuffle(order.begin(), order.end(), random);
            for (int index : order) {
                const auto begin = std::chrono::steady_clock::now();
                const auto& p = problems[index]; const auto f = labor::extract(p);
                const auto menu = labor::earliest_menu(p);
                const auto supply = with_supply ? labor::supply_bound(p, menu) : labor::SupplyBound{};
                const int lower = std::max(supply.workers, labor::workforce_lower_bound(p, f, menu));
                const auto middle = std::chrono::steady_clock::now();
                const double cost = labor::search_model::cost(f);
                const auto end = std::chrono::steady_clock::now();
                const double feature_us = std::chrono::duration<double, std::micro>(middle - begin).count();
                const double cost_us = std::chrono::duration<double, std::micro>(end - middle).count();
                for (int workers = 1; workers <= 40; ++workers) {
                    const auto first = std::chrono::steady_clock::now();
                    const auto x = labor::search_model::query_features(f, workers, lower);
                    const double logistic = labor::search_model::logistic(x), boost = labor::search_model::boost(x);
                    const double cpu = std::max(.0001, labor::search_model::cpu(x));
                    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - first).count();
                    predictions.push_back({index, pass, workers, lower, supply.missing, f[labor::deadline_missing_quantity] > 0,
                                           cost, logistic, boost, cpu, feature_us, cost_us, us});
                }
            }
        }
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot open output");
        output << "id,pass,workers,lower_bound,input_missing,output_missing,cost,logistic,boost,cpu,feature_us,cost_us,query_us\n" << std::setprecision(17);
        for (const auto& r : predictions)
            output << ids[r.input] << ',' << r.pass << ',' << r.workers << ',' << r.lower << ',' << r.input_missing << ',' << r.output_missing << ',' << r.cost << ',' << r.logistic << ',' << r.boost << ',' << r.cpu << ',' << r.feature_us << ',' << r.cost_us << ',' << r.query_us << '\n';
        std::cout << predictions.size() << " mixed-input C++ query predictions\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
