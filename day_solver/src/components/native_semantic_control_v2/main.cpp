#include "adapter.hpp"
#include <iostream>
using namespace semantic_adapter;

int main(int argc, char** argv) {
    if (argc < 4) { std::cerr << "usage: semantic_control_cli v3_problem internal_proposal output [options]\n"; return 2; }
    const fs::path output = argv[3];
    bool owns_output = false;
    try {
        semantic::Options options;
        for (int i = 4; i < argc; ++i) {
            const std::string flag = argv[i];
            if (flag == "--fix-source-profiles" || flag == "--defer-infeasible-owner-repair") set_option(options, flag, 1);
            else { require(i + 1 < argc, "missing value"); set_option(options, flag, std::stod(argv[++i])); }
        }
        require(!fs::exists(output / "report.json") && !fs::exists(output / "candidate.json"), "output contains prior result");
        fs::create_directories(output); owns_output = true;
        semantic::Session session(day_solver::load_problem_json(argv[1]));
        auto result = session.repair(read_hint(read_json(argv[2])), options);
        auto report = result_json(result);
        report["candidate"] = nullptr;
        report["model"] = "native_semantic_control";
        report["scope"] = "Internal repair of our generated proposal; public construction/portfolio is a separate controller";
        if (result.accepted()) {
            write_json(output / "candidate.json", schedule_json(*result.exact->schedule));
            write_json(output / "route_hint.json", hint_json(*result.winning_hint));
            report["candidate"] = (output / "candidate.json").string();
        }
        write_json(output / "report.json", report);
        std::cout << json::serialize(report) << '\n';
        return result.accepted() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (owns_output) write_json(output / "report.json", json::object{{"status", "MODEL_REJECTED"}, {"reason", error.what()}});
        return 2;
    }
}
