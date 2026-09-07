#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "google/protobuf/text_format.h"
#include "ortools/sat/cp_model_checker.h"
#include "ortools/sat/cp_model_solver.h"

using namespace operations_research::sat;

template <class Message>
bool read_text(const char* path, Message* message) {
    std::ifstream stream(path);
    if (!stream) return false;
    const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    return google::protobuf::TextFormat::ParseFromString(text, message);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: cp_sat_runner model.textproto parameters.textproto response.textproto\n";
        return 2;
    }
    CpModelProto problem;
    SatParameters parameters;
    if (!read_text(argv[1], &problem) || !read_text(argv[2], &parameters)) return 2;
    const auto error = ValidateCpModel(problem);
    if (!error.empty()) {
        std::cerr << error << '\n';
        return 2;
    }
    Model model;
    model.Add(NewSatParameters(parameters));
    const auto response = SolveCpModel(problem, &model);
    if ((response.status() == OPTIMAL || response.status() == FEASIBLE) &&
        !SolutionIsFeasible(problem, {response.solution().data(), std::size_t(response.solution_size())})) {
        std::cerr << "invalid CP assignment\n";
        return 2;
    }
    std::string text;
    if (!google::protobuf::TextFormat::PrintToString(response, &text)) return 2;
    std::ofstream output(argv[3]);
    output << text;
    if (!output) return 2;
    std::cout << CpSolverStatus_Name(response.status()) << '\n';
    return 0;
}
