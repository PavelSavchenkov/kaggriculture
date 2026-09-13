#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include <boost/json.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace adapter {
namespace json = boost::json;
namespace fs = std::filesystem;
constexpr int hours = day_solver::HOURS;
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
#include "components/native_solver_api/json_output.hpp"
std::string digest(const std::string& value) {
    unsigned char bytes[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), bytes);
    std::ostringstream result;
    for (unsigned char byte : bytes) result << std::hex << std::setw(2) << std::setfill('0') << int(byte);
    return result.str();
}
}

int main(int argc, char** argv) {
    using namespace adapter;
    try {
        require(argc >= 3 && argc <= 6, "usage: day_solver_cli input.json output [seconds [fallback_threads [portfolio|regret|regret-deferred|regret-fast]]]");
        const auto problem = day_solver::load_problem_json(argv[1]);
        const fs::path output(argv[2]);
        require(!fs::exists(output) || (fs::is_directory(output) && fs::is_empty(output)), "output must be new or empty");
        fs::create_directories(output);
        day_solver::save_problem_json(problem, output / "input.json");
        day_scheduler::Options options;
        if (argc >= 4) options.seconds = std::stod(argv[3]);
        if (argc >= 5) options.fallback_workers = std::stoi(argv[4]);
        if (argc >= 6) {
            const std::string search = argv[5];
            require(search == "portfolio" || search == "regret" || search == "regret-deferred" || search == "regret-fast",
                "search must be portfolio, regret, regret-deferred or regret-fast");
            if (search == "regret") options.search = day_scheduler::Search::Regret;
            if (search == "regret-deferred") options.search = day_scheduler::Search::RegretDeferred;
            if (search == "regret-fast") options.search = day_scheduler::Search::RegretFast;
        }
        const auto result = day_scheduler::solve(problem, options);
        json::array stages;
        for (const auto& stage : result.stages)
            stages.push_back(json::object{{"stage", stage.name}, {"status", stage.status}, {"seconds", stage.seconds}});
        json::object report{{"status", result.schedule ? "SCHEDULE" : "UNKNOWN"},
            {"seconds", result.seconds}, {"backend", options.search == day_scheduler::Search::RegretFast ? "regret_jobs_fast" :
                options.search == day_scheduler::Search::RegretDeferred ? "regret_jobs_deferred" :
                options.search == day_scheduler::Search::Regret ? "regret_jobs" : "v30_capacity"},
            {"input_sha256", digest(day_solver::serialize_problem_json(problem))},
            {"proof", json::object{{"proven_infeasible", false}}},
            {"stages", std::move(stages)},
            {"schedule", result.schedule ? json::value("schedule.json") : json::value(nullptr)}};
        if (result.schedule) {
            const auto replay = day_solver::replay_schedule(problem, *result.schedule);
            require(replay.requirements_satisfied && replay.invariants_satisfied &&
                    replay.candidate.replay.strict_valid && replay.errors.empty(), "returned schedule failed replay");
            write_json(output / "schedule.json", schedule_json(*result.schedule));
        }
        write_json(output / "report.json", report);
        std::cout << json::serialize(report) << '\n';
        return result.schedule ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
