#include "quick.hpp"
#include "../native_semantic_control_v2/adapter.hpp"
#include <openssl/sha.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace adapter {
using namespace semantic_adapter;
std::string read_text(const fs::path& path) {
    std::ifstream input(path);
    require(bool(input), "cannot read " + path.string());
    return {std::istreambuf_iterator<char>(input), {}};
}
std::string digest(const std::string& value) {
    unsigned char bytes[SHA256_DIGEST_LENGTH];
    require(SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), bytes), "SHA-256 failed");
    std::ostringstream result;
    for (unsigned char byte : bytes) result << std::hex << std::setw(2) << std::setfill('0') << int(byte);
    return result.str();
}
json::array attempts_json(const std::vector<portfolio::Attempt>& attempts) {
    static constexpr const char* kinds[] = {"constructor", "repair", "semantic", "screen", "exact", "conservation"};
    json::array values;
    for (const auto& attempt : attempts) {
        semantic::Result semantic;
        semantic.events = attempt.semantic_events;
        values.push_back(json::object{{"stage", attempt.stage}, {"kind", kinds[int(attempt.kind)]},
            {"solver_status", sat::CpSolverStatus_Name(attempt.status)}, {"seconds", attempt.seconds},
            {"error", attempt.error}, {"semantic_events", result_json(semantic).at("events")}});
    }
    return values;
}
} // namespace adapter

int main(int argc, char** argv) {
    using namespace adapter;
    if (argc < 3) { std::cerr << "usage: day_solver_cli v3_problem output [--time-limit seconds] [--fast-seconds seconds] [--exact-workers count] [--early-completion-seconds seconds]\n"; return 2; }
    const fs::path output = argv[2];
    bool owns_output = false;
    try {
        quick_v30::Options options;
        for (int index = 3; index < argc; ++index) {
            const std::string flag = argv[index];
            require(index + 1 < argc, "missing option value");
            const std::string argument = argv[++index];
            std::size_t used = 0;
            const double value = std::stod(argument, &used);
            require(used == argument.size() && std::isfinite(value) && value >= 0, "invalid option value");
            if (flag == "--exact-tuning" || flag == "--screen-tuning" || flag == "--rounds") {
                require(value == std::floor(value) && value <= 4, "invalid tuning/count");
                if (flag == "--exact-tuning") options.exact_tuning = int(value);
                else if (flag == "--screen-tuning") options.screen_tuning = int(value);
                else options.rounds = int(value);
            } else if (flag == "--iterations" || flag == "--variants") {
                require(value == std::floor(value) && value <= 100000, "invalid count");
                if (flag == "--iterations") options.iterations = int(value);
                else options.variants = int(value);
            } else if (flag == "--route-seconds") options.route_seconds = value;
            else if (flag == "--completion-seconds") options.completion_seconds = value;
            else if (flag == "--screen-seconds") options.screen_seconds = value;
            else if (flag == "--geometry-seconds") options.geometry_seconds = value;
            else if (flag == "--geometry-after") {
                require(value >= 1 && value <= 16 && value == std::floor(value), "invalid geometry trigger");
                options.geometry_after = int(value);
            }
            else if (flag == "--retry-seconds") options.retry_seconds = value;
            else if (flag == "--compact") { require(value == 0 || value == 1, "invalid boolean"); options.compact = value; }
            else if (flag == "--materialize") { require(value == 0 || value == 1, "invalid boolean"); options.materialize = value; }
            else if (flag == "--modern-fallback") { require(value == 0 || value == 1, "invalid boolean"); options.modern_fallback = value; }
            else if (flag == "--fallback") { require(value == 0 || value == 1, "invalid boolean"); options.fallback = value; }
            else if (flag == "--time-limit") options.reference.seconds = value;
            else if (flag == "--fast-seconds") options.reference.fast_seconds = value;
            else if (flag == "--early-completion-seconds") options.reference.early_seconds = value;
            else if (flag == "--exact-workers") {
                require(value > 0 && value == std::floor(value) && value <= std::numeric_limits<int>::max(), "invalid worker count");
                options.reference.workers = int(value);
            } else throw std::runtime_error("unsupported option: " + flag);
        }
        require(!fs::exists(output) || fs::is_empty(output), "use a new or empty output directory");
        const auto supplied = read_text(argv[1]);
        const auto problem = day_solver::parse_problem_json(supplied);
        fs::create_directories(output); owns_output = true;
        {
            std::ofstream copy(output / "input.json");
            copy << supplied;
            require(bool(copy), "cannot write input snapshot");
        }
        const auto result = quick_v30::solve(problem, options);
        require(read_text(output / "input.json") == supplied, "input snapshot changed during solve");
        json::object report{{"status", result.accepted() ? "SCHEDULE" : "UNKNOWN"},
            {"seconds", result.seconds}, {"input_sha256", digest(supplied)},
            {"attempts", attempts_json(result.attempts)}, {"winning_stage", result.winning_stage},
            {"schedule", result.accepted() ? json::value("schedule.json") : json::value(nullptr)},
            {"proof", {{"proven_infeasible", false}}}, {"backend", "native_quick_portfolio_v30"}};
        if (result.accepted()) {
            write_json(output / "schedule.json", schedule_json(*result.winner->schedule));
            report["exact_report"] = exact_json(*result.winner);
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
