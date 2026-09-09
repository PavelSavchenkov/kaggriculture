#include "bounds.hpp"
#include "model.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>

struct Record {
    int index, pass, bound; bool impossible;
    double formula, ridge, trees, feature_us, model_us;
};

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: predict manifest.txt output.csv repeats");
        std::ifstream input(argv[1]); const int repeats = std::stoi(argv[3]);
        if (!input || repeats < 1) throw std::runtime_error("invalid arguments");
        std::vector<std::string> ids; std::vector<day_solver::DayProblem> problems;
        std::string id, path;
        while (input >> id >> path) { ids.push_back(id); problems.push_back(day_solver::load_problem_json(path)); }
        if (!input.eof()) throw std::runtime_error("invalid manifest");
        std::vector<int> order(ids.size()); std::iota(order.begin(), order.end(), 0);
        std::vector<Record> rows; rows.reserve(ids.size() * repeats);
        std::mt19937 random(909);
        for (int pass = 0; pass < repeats; ++pass) {
            std::shuffle(order.begin(), order.end(), random);
            for (int index : order) {
                const auto begin = std::chrono::steady_clock::now();
                const auto& p = problems[index]; const auto f = labor::extract(p);
                const int bound = labor::workforce_lower_bound(p, f, labor::earliest_menu(p));
                const auto middle = std::chrono::steady_clock::now();
                const double a = labor::model::formula(f), b = labor::model::ridge(f), c = labor::model::trees(f);
                const auto end = std::chrono::steady_clock::now();
                rows.push_back({index, pass, bound, f[labor::deadline_missing_quantity] > 0, a, b, c,
                               std::chrono::duration<double, std::micro>(middle - begin).count(),
                               std::chrono::duration<double, std::micro>(end - middle).count()});
            }
        }
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot create output");
        output << "id,pass,lower_bound,impossible,formula,ridge,trees,feature_us,model_us,combined_us\n" << std::setprecision(12);
        for (const auto& r : rows) output << ids[r.index] << ',' << r.pass << ',' << r.bound << ',' << r.impossible
            << ',' << r.formula << ',' << r.ridge << ',' << r.trees << ',' << r.feature_us << ',' << r.model_us << ',' << r.feature_us + r.model_us << '\n';
        std::cout << rows.size() << " mixed-input predictions; all three models evaluated per input\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
