#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace kag::agents::day_policy_contract {
constexpr int MAX_WORKERS = 14; // Farmer plus the contract's 13-hire cap.
constexpr int MAX_JOBS = 512;
constexpr int MAX_STEPS = 16;
constexpr int BIND_NEW_WORKERS = 2048;
constexpr int EXACT_BIRTH_TIMING = 67108864;
constexpr int SHARED_DROP_SCORE = 16384;
constexpr int FIELD_FERTILIZER = 8388608;
constexpr int FINISH_TILE_BEFORE_RETURN = 16777216;
constexpr int REORDER_ROUTE_JOBS = 33554432;
constexpr int REASSIGN_ROUTE_JOBS = 1073741824;
constexpr int ANIMAL_SERVICE_SEED = 131072;
constexpr int BEAM_ASSIGNMENT_SEED = 524288;
constexpr int EXCHANGE_ROUTE_TAILS = 1048576;
constexpr int SEGMENTED_PICKUPS = 2097152;
constexpr int URGENT_RETURNS = 4194304;
constexpr int REGRET_ASSIGNMENT = 262144;
constexpr int AUXILIARY_SOURCE = -2;
enum class PlacementStyle { Legacy, Staged };

struct Job {
    int16_t tile = -1;
    int16_t key = -1; // Caller identity; never used for ranking.
    uint8_t count = 0;
    UnitAction steps[MAX_STEPS]{};
    uint8_t deadline = 23;
    bool new_site = false;
    bool depot = false;
    int16_t predecessor = -1;
    bool carrier_bound = false;
    int16_t service_predecessor = -1; // May be completed by another worker.
    uint8_t prior_service = 0; // Project requested fertilizer/water before harvest.
};

struct DayPlan {
    int day = -1;
    int hires = 11;
    int count = 0;
    Job jobs[MAX_JOBS]{};
    int buy_seeds[N_CROPS]{};
    int buy_items[N_ITEMS]{};
    int buy_land = 0;
    int land_hour = -1;
    int sell_target[24][N_PRODUCTS]{}; // Cumulative desired sales by hour.
    bool relocate_new = false;
    bool trade = true;
    int first_wave = -1;
    int delivery_batch_cap = 0;
    std::array<int8_t,MAX_WORKERS> spawn_tile = [] { std::array<int8_t,MAX_WORKERS> a; a.fill(-1); return a; }();
};

// Cumulative field receipts into the shed, excluding its dawn stock.
// A sale target already implies receipts; use this for worker resupply or storage.
struct DayReturns { int target[24][N_PRODUCTS]{}; };

struct Options {
    int batch = 3;
    int animal_bias = 2;
    int fertilizer_bias = 2;
    int outward = 1;
    int delivery_lead = 2;
    int region_penalty = 0;
    int work_bias = 8;
    int far_start = 0;
    int route_rounds = 1;
    int return_source_seed = 0;
    bool reachable_sources = true;
    bool deadline_deposits = false;
    int route_variant = 24 | BIND_NEW_WORKERS | FIELD_FERTILIZER; // Delivery batches, slack, actual hire positions.
    bool route_repair = true;
    bool preposition = false;
    bool relocate_idle = false;
    bool match_routes = false;
    bool replan_after_hires = false;
    bool route_delivery = true;
    bool reserve_hire_cash = true;
    bool dawn_inputs = true; // Seeds and animals; wheat may also be prepared at dawn.
    int auto_hires = -1; // -1: supplied workforce; otherwise reserve this many extra hires.
};

struct Routes {
    Routes() {} // Unused job slots are never read; initialize only counts and diagnostics.
    int8_t start[MAX_WORKERS]{};
    int count[MAX_WORKERS]{};
    int16_t jobs[MAX_WORKERS][MAX_JOBS];
    int predicted_length[MAX_WORKERS]{};
    int overrun = 0;
    uint64_t evaluations = 0;
    bool truncated = false;
};

struct Progress {
    int completed = 0;
    int requested = 0;
    int noops = 0;
    int moves = 0;
    int deposits = 0;
    int auxiliary = 0;
    int placements[100]{};
};

int first_hire_wave(const DayPlan& plan, int order_limit);
struct Geometry {
    uint8_t distance[100][100]{}, shed[100]{}, corner[100]{};
    constexpr Geometry() {
        for (int a = 0; a < 100; ++a) {
            const int x = a % 10, y = a / 10;
            const int sx = x < 4 ? 4 : x > 5 ? 5 : x;
            const int sy = y < 4 ? 4 : y > 5 ? 5 : y;
            corner[a] = sy * 10 + sx;
            for (int b = 0; b < 100; ++b) {
                const int dx = x - b % 10, dy = y - b / 10;
                distance[a][b] = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            }
            shed[a] = distance[a][corner[a]];
        }
    }
};
inline constexpr Geometry GEOMETRY{};
inline int distance(int a, int b) { return GEOMETRY.distance[a][b]; }
inline int shed_distance(int cell) { return GEOMETRY.shed[cell]; }
inline int shed_cell(int cell) { return GEOMETRY.corner[cell]; }
int choose_site(const Tile* tiles, const bool* reserved, int worker_cell, bool animal, int structure = -1);
int harvest_output(const Job& job, int cursor, Tile tile, int day, int product);
int wheat_harvest(const Job& job, int cursor, Tile tile, int day);

// Observation-only executor. An external controller may supply set_plan at dawn.
// Without one, a small maintenance/planting controller supports API smoke tests.
class Agent {
public:
    static agent::AgentInfo info();
    void reset(const agent::AgentInit& init);
    void act(const agent::AgentObservation& observation, const agent::DecisionBudget& budget, Action& action);
    void set_plan(const DayPlan& plan);
    void prepare_routes(const agent::AgentObservation& observation, const agent::DecisionBudget& budget);
    void set_returns(const DayReturns& returns) { assert_returns(returns); returns_ = returns; returns_supplied_ = true; }
    void set_options(Options options) { options_ = options; }
    void set_fixed_orders(const Action* actions) {
        fixed_orders_ = true;
        for (int h = 0; h < 24; ++h) fixed_actions_[h] = actions[h];
    }
    const Progress& progress() const { return progress_; }
    const DayPlan& plan() const { return plan_; }
    int completed_steps(int job) const { return cursor_[job]; }
    int predicted_overrun() const { return routes_ready_ ? routes_.overrun : -1; }
    const Routes& route_diagnostics() const { return routes_; }
    int predicted_length(int worker) const { return routes_ready_ ? routes_.predicted_length[worker] : -1; }

private:
    void record_deposit(int worker, int* quantities);
    struct Worker { int job = -1; int next = -1; int returning = -1; };
    struct Choice { int job = -1; int source = -1; int site = -1; int cost = 1000000; int transfer_end = -1; };
    agent::AgentInit init_{};
    DayPlan plan_{};
    DayReturns returns_{};
    bool returns_supplied_ = false;
    bool fixed_orders_ = false;
    Action fixed_actions_[24]{};
    int returned_[N_PRODUCTS]{};
    static void assert_returns(const DayReturns& returns);
    Options options_{};
    Progress progress_{};
    Worker workers_[MAX_WORKERS]{};
    uint8_t cursor_[MAX_JOBS]{};
    int16_t owner_[MAX_JOBS]{};
    bool reserved_[100]{};
    Farm local_{}; // Only observing player's public/private data, copied from observation.
    int bought_seeds_[N_CROPS]{};
    int bought_items_[N_ITEMS]{};
    int sold_[N_PRODUCTS]{};
    int bought_land_ = 0;
    int hour_ = 0;
    int end_hour_ = 23;
    int remaining_feed_ = 0;
    int remaining_fertilize_ = 0;
    int previous_seeds_[N_CROPS]{};
    int previous_items_[N_ITEMS]{};
    int pending_seeds_[N_CROPS]{};
    int pending_items_[N_ITEMS]{};
    int previous_sold_[N_PRODUCTS]{};
    int previous_land_ = 1;
    bool fresh_ = true;
    int8_t region_[MAX_WORKERS]{};
    int region_work_[4]{};
    int worker_done_[MAX_WORKERS]{};
    Routes routes_{};
    bool routes_ready_ = false;
    bool replanned_ = false;
    int known_units_ = 0;
    void automatic_plan(const agent::AgentObservation& observation);
    void load_observation(const agent::AgentObservation& observation);
    void assign_sites();
    void bind_new_workers();
    bool ready(int job) const;
    int work_left(int job) const;
    Choice choose(int worker);
    Choice repair(int worker);
    Choice relocate_crop(int worker);
    UnitAction next_action(int worker);
    bool apply(int worker, UnitAction action);
    int supply_cost(int worker, const Job& job) const;
    void orders(const agent::AgentObservation& observation, Action& action);
};
static_assert(agent::LocalAgent<Agent>);
}
