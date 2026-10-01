#pragma once
#include "agent.hpp"
#include <climits>

namespace kag::agents::day_policy_contract {
enum Event : uint8_t {
    Water = 1, Fertilize = 2, Harvest = 4, Clear = 8,
    Feed = 16, Care = 32, CollectFertilizer = 64
};
struct NewProduct { uint8_t product = 0, events = 0; };

// Tile times are relative to this dawn: planted_day = -age,
// fertilized_until_day = remaining active days - 1, max_lifespan_step =
// hours until decay starts (INT_MAX when unset). No calendar day is required.
struct DayInput {
    Tile grid[100]{};
    int shed[N_ITEMS]{}, seeds[N_CROPS]{};
    int hours = 24;
    int buy_seeds[24][N_CROPS]{}, buy_animals[24][3]{};
    int buy_wheat[24]{}, buy_fertilizer[24]{};
    int land_hour = -1;
    uint8_t events[100]{};
    NewProduct establish[100]{};
    int establish_count = 0;
    int returns[24][N_PRODUCTS]{};
};

struct WorkerState { uint8_t tile = 44; int inventory[N_ITEMS]{}; };
// State after the final actions and decay, before automatic night settlement.
// The caller applies night settlement and observes the next random weed spawn.
struct DayState {
    Tile grid[100]{};
    int shed[N_ITEMS]{}, seeds[N_CROPS]{};
    WorkerState workers[14]{};
    int worker_count = 1;
};
enum class SolveStatus { Success, InvalidInput, NoScheduleFound };
struct SolveResult {
    SolveStatus status = SolveStatus::NoScheduleFound;
    Action schedule[24]{};
    DayState state{};
    int hires = 0;
    int receipts[24][N_PRODUCTS]{};
    int production[N_PRODUCTS]{};
    int attempts = 0;
    int executions = 0;  // local: route executions (24-hour simulations) used
    double microseconds = 0;
};
enum class SearchEffort { Fast, Full, Compact, Balanced, Classic, DayPolicy80p };
struct SolveOptions {
    SearchEffort effort = SearchEffort::Balanced;
    int max_hires = 13;
    int variants = 4; // 16 also tries finishing same-tile work before returning.
    int route_rounds = 1;
    bool minimize_hires = true;
    bool opportunistic_hire_reduction = false;
    int minimize_variants = 1; // Use 8 for a more expensive workforce search.
    int animal_reserve = 0;
    int min_hires = 0;  // local: start the hire search here (re-solves of a solved day)
    int max_executions = 0;  // local: give up after this many route executions (0: no limit)
    // local: search the largest workforce first (an infeasible day costs one hire level), then
    // reduce hires, trying the winning configuration first at each level.
    bool feasibility_first = false;
    PlacementStyle placement = PlacementStyle::Staged;
};
// Fixed 11-hire throughput profile. Do not change max_hires or minimize_hires.
SolveOptions day_policy_80p();
// Fast profile for the unrestricted within-day purchase, fertilizer and hire contract.
SolveOptions unrestricted_day_policy();

// Local: time per solver stage (ms, this thread) for compile-time profiling.
struct SolverProfile {
    double setup = 0, prepare = 0, execute = 0, retry_execute = 0;
    double route_setup = 0, route_init = 0, route_improve = 0, route_reorder = 0;
    double reorder_refine = 0, reorder_reassign = 0, reorder_exchange = 0;
    long executions = 0, retry_executions = 0;
};
SolverProfile& solver_profile();

class Solver {
public:
    SolveResult solve(const DayInput& input, const SolveOptions& options = {});
private:
    Agent executor_, prepared_;
};
}
