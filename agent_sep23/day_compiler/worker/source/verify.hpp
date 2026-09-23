#pragma once
#include "policy.hpp"

namespace kag::agents::sep22_worker {
// Independent native-engine check, also used by the replay benchmark.
// NoScheduleFound does not assert mathematical infeasibility.
struct Verification {
    bool valid = false;
    int failed_hour = -1;
    const char* reason = "unchecked";
    int receipts[24][N_PRODUCTS]{};
};
Verification verify(const DayInput& input, const SolveResult& result);
// Re-evaluate a previously available program from the actual suffix state.
// This checks the same full contract; it never assumes the old plan still works.
Verification replay_schedule(const DayInput& input,const Action* schedule,SolveResult& result,
                             const SolveOptions& options={});
namespace detail {
constexpr int CALENDAR = 40;
Sim initial_state(const DayInput& input);
DayState export_state(const Sim& sim);
void advance(Sim& sim, const Action& action, int hour);
bool valid_input(const DayInput& input);
}
}
