#pragma once
#include "agent.hpp"
#include <climits>

namespace kag::agents::sep22_worker {
enum Event : uint8_t {
    Water = 1, Fertilize = 2, Harvest = 4, Clear = 8,
    Feed = 16, Care = 32, CollectFertilizer = 64
};
struct NewProduct { uint8_t product = 0, events = 0; };
struct WorkerState {
    uint8_t tile = 44;
    int inventory[N_ITEMS]{};
    uint8_t inventory_keys[N_ITEMS]{}, inventory_count=0;
};

// Tile times are relative to this dawn: planted_day = -age,
// fertilized_until_day = remaining active days - 1, max_lifespan_step =
// hours until decay starts (INT_MAX when unset). No calendar day is required.
struct DayInput {
    Tile grid[100]{};
    int shed[N_ITEMS]{}, seeds[N_CROPS]{};
    int hours = 24;
    int start_hour = 0, worker_count = 1;
    WorkerState workers[MAX_WORKERS]{};
    int buy_seeds[24][N_CROPS]{}, buy_animals[24][3]{};
    int buy_wheat[24]{}, buy_fertilizer[24]{};
    int land_hour = -1;
    uint8_t events[100]{};
    NewProduct establish[100]{};
    int establish_count = 0;
    int returns[24][N_PRODUCTS]{}; // Cumulative minimum worker deposits; extra deposits are allowed.
};

// State after the final actions and decay, before automatic night settlement.
// The caller applies night settlement and observes the next random weed spawn.
struct DayState {
    Tile grid[100]{};
    int shed[N_ITEMS]{}, seeds[N_CROPS]{};
    WorkerState workers[MAX_WORKERS]{};
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
    double microseconds = 0;
};
enum class SearchEffort { Fast, Full, Compact, Balanced, Classic, DayPolicy80p };
struct SolveOptions {
    uint64_t max_attempts = UINT64_MAX;
    agent::DecisionBudget budget{};
    int reserved_order_slots = 0;
    std::array<uint8_t, MAX_WORKERS> hire_not_before{};
    std::array<uint8_t, 100> harvest_deadline = [] { std::array<uint8_t, 100> a; a.fill(23); return a; }();
    SearchEffort effort = SearchEffort::Balanced;
    int max_hires = 13;
    int first_hires = -1;
    int variants = 4; // 16 also tries finishing same-tile work before returning.
    int route_rounds = 1;
    bool minimize_hires = true;
    bool opportunistic_hire_reduction = false;
    int minimize_variants = 1; // Use 8 for a more expensive workforce search.
    int animal_reserve = 0;
    PlacementStyle placement = PlacementStyle::Staged;
};
// Fixed 11-hire throughput profile. Do not change max_hires or minimize_hires.
SolveOptions day_policy_80p();
// Fast profile for the unrestricted within-day purchase, fertilizer and hire contract.
SolveOptions unrestricted_day_policy();

class Solver {
public:
    SolveResult solve(const DayInput& input, const SolveOptions& options = {});
private:
    Agent executor_, prepared_;
};
}
