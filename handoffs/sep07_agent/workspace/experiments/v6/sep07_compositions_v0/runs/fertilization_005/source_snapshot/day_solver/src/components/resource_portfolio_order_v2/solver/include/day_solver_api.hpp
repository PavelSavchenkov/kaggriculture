#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "fast_game_engine/sim.hpp"

namespace day_solver {

constexpr uint32_t FORMAT_VERSION = 3;
constexpr int HOURS = 24;

// Inputs are signed-32-bit counts; within-day totals can exceed that range.
using InventoryCount = int64_t;
constexpr InventoryCount MAX_INPUT_COUNT = std::numeric_limits<int32_t>::max();
constexpr int16_t NO_SUBJECT = -1;
constexpr int16_t NO_TILE = -1;

// Omitted board squares remain traversable but cannot receive farm actions.
enum class ManagedTileKind : uint8_t {
    EMPTY,
    WEED,
    PASTURE,
    CROP,
    COOP,
    LOCKED,
};

constexpr bool animal_structure(ManagedTileKind kind) {
    return kind == ManagedTileKind::PASTURE || kind == ManagedTileKind::COOP;
}

constexpr ManagedTileKind structure_for(int animal) {
    return animal == kag::GOOSE ? ManagedTileKind::COOP : ManagedTileKind::PASTURE;
}

// A day-independent, lossless representation of all state that can affect the
// current deterministic day. Age and fertilizer duration are relative so the
// input does not need a day index.
struct ManagedTileState {
    ManagedTileKind kind = ManagedTileKind::EMPTY;
    int16_t crop = NO_SUBJECT;
    int16_t animal = NO_SUBJECT;
    int64_t age_days = 0;
    int16_t stored_units = 0;
    int64_t consecutive_dry_days = 0;
    int64_t pending_care_bonus = 0;
    int64_t fertilizer_days_remaining = 0;
    bool watered_today = false;
    bool fed_today = false;
    bool cared_today = false;
    bool fertilizer_available = false;

    friend bool operator==(const ManagedTileState&,
                           const ManagedTileState&) = default;
};

struct ManagedTile {
    int8_t x = 0;
    int8_t y = 0;
    ManagedTileState state;

    friend bool operator==(const ManagedTile&, const ManagedTile&) = default;
};

struct PhysicalState {
    std::vector<ManagedTile> managed_tiles;
    std::array<InventoryCount, kag::N_ITEMS> shed{};
    std::array<InventoryCount, kag::N_CROPS> seeds{};
    int16_t shed_capacity = 100;
    int64_t cash = 0;

    friend bool operator==(const PhysicalState&, const PhysicalState&) = default;
};

enum class OutcomeMetric : uint8_t {
    PLANTED,
    HARVESTED,
    ANIMAL_PLACED,
    FED,
    CARED,
    FERTILIZER_COLLECTED,
    CROP_WATERED,
    CROP_FERTILIZED,
    PASTURE_BUILT,
    DUG,
    PRODUCT_DEPOSITED,
    END_TILE_STORED,
    END_SHED,
    END_SEEDS,
    END_CASH,
    HIRES,
    PURCHASED,
    PICKUPS,
    DROPS,
    UNIT_ACTIONS,
    TRAVEL,
    SPARE_TURNS,
    MIN_SHED_HEADROOM,
    COOP_BUILT,
};

// subject is a crop, animal, or product as determined by metric. tile is the
// managed_tiles index, not a board index. -1 aggregates all subjects/tiles.
// Event counters use inclusive through_hour [0, 23]; end metrics require -1.
struct OutcomeKey {
    OutcomeMetric metric{};
    int16_t subject = NO_SUBJECT;
    int16_t tile = NO_TILE;
    int8_t through_hour = -1;

    friend bool operator==(const OutcomeKey&, const OutcomeKey&) = default;
};

struct OutcomeBound {
    OutcomeKey key;
    int64_t lower = std::numeric_limits<int64_t>::min();
    int64_t upper = std::numeric_limits<int64_t>::max();
};

// kind and item are derived conveniences for pattern compilers. exact_state is
// the authoritative required successor state in format-v2 JSON.
struct EndTileRequirement {
    int16_t tile = NO_TILE;
    ManagedTileKind kind = ManagedTileKind::EMPTY;
    int16_t item = NO_SUBJECT;
    std::optional<ManagedTileState> exact_state;
};

struct TileWorkAction {
    uint8_t op = kag::OP_PASS;
    int16_t arg = NO_SUBJECT;
    int32_t quantity = 1;
    int16_t output_item = NO_SUBJECT;
    int32_t output_quantity = 0;
};

struct TileWork {
    int16_t tile = NO_TILE;
    std::vector<TileWorkAction> actions;
};

struct SaleTarget {
    int16_t item = NO_SUBJECT;
    int32_t maximum_quantity = 0;
    // Index of a SELL event in market_plan. item and maximum_quantity are
    // resolved from that event and retained for solver internals.
    uint16_t market_event = std::numeric_limits<uint16_t>::max();
};

// An exact successful external market event. cash_delta is the authoritative
// signed change after the complete event; prices and opponent orders are not
// part of the physical day problem.
struct MarketEvent {
    int8_t hour = 0;
    int8_t order_index = 0;
    uint8_t market_op = kag::M_NONE;
    int16_t item = NO_SUBJECT;
    int32_t quantity = 0;
    int64_t cash_delta = 0;
};

// Acquisitions are optional within [lower, upper] and may execute only in the
// inclusive hour range. BUY_PRODUCT has one committed cost for every allowed
// unit; other costs are fixed by global rules and leave that vector empty.
struct AllowedAcquisition {
    uint8_t market_op = kag::M_NONE;
    int16_t item = NO_SUBJECT;
    int32_t lower = 0;
    int32_t upper = 0;
    int8_t first_hour = 0;
    int8_t last_hour = HOURS - 1;
    std::vector<int32_t> committed_unit_costs;
};

struct SolveLimits {
    uint64_t node_limit = 0;
    uint64_t memory_limit_bytes = 0;
    uint32_t frontier_limit = 0;
};

struct DayProblem {
    uint32_t format_version = FORMAT_VERSION;
    PhysicalState start;
    // Exact total workforce, including the one farmer present at hour 0.
    // The schedule must hire worker_count - 1 hands during the day.
    uint16_t worker_count = 1;
    std::vector<OutcomeBound> required_outcomes;
    std::vector<EndTileRequirement> required_end_tiles;
    std::vector<TileWork> tile_work;
    std::array<std::array<InventoryCount, kag::N_ITEMS>, HOURS>
        shed_availability{};
    std::array<InventoryCount, kag::N_ITEMS> end_shed{};
    std::array<InventoryCount, kag::N_CROPS> end_seeds{};
    std::vector<MarketEvent> market_plan;
    std::vector<SaleTarget> sale_targets;
    // Derived compatibility view for the current optimization engines. It is
    // not part of format-v2 JSON and will be removed after their migration.
    std::vector<AllowedAcquisition> allowed_acquisitions;
    SolveLimits limits;
};

struct WorkerState {
    int8_t x = 4;
    int8_t y = 4;
    std::array<InventoryCount, kag::N_ITEMS> cargo{};
    // Required because capacity-bound deposit follows insertion order.
    std::vector<uint8_t> cargo_order;

    friend bool operator==(const WorkerState&, const WorkerState&) = default;
};

struct DayEndState {
    PhysicalState physical;
    std::vector<WorkerState> workers;

    friend bool operator==(const DayEndState&, const DayEndState&) = default;
};

struct OutcomeValue {
    OutcomeKey key;
    int64_t value = 0;
};

struct SaleAvailability {
    uint16_t market_event = std::numeric_limits<uint16_t>::max();
    int16_t item = NO_SUBJECT;
    // Element k is the earliest hour after worker actions when at least k+1
    // units are present in the shed. -1 means unavailable during the day.
    std::vector<int8_t> unit_hours;
};

struct HourSummary {
    int8_t hour = 0;
    std::array<InventoryCount, kag::N_ITEMS> shed_after_workers{};
    std::array<InventoryCount, kag::N_ITEMS> shed_after_market{};
    InventoryCount headroom_after_workers = 0;
    InventoryCount headroom_after_market = 0;
    std::vector<OutcomeValue> cumulative_outcomes;
};

struct PhysicalCosts {
    uint32_t unit_actions = 0;
    uint32_t travel_actions = 0;
    uint32_t hires = 0;
    uint64_t purchased_units = 0;
    int64_t acquisition_cost = 0;
    uint64_t sold_units = 0;
    int64_t sale_proceeds = 0;
    uint32_t pickups = 0;
    uint32_t drops = 0;
    uint32_t spare_turns = 0;
};

enum class SolveStatus : uint8_t {
    OPTIMAL,
    GAP_BOUNDED,
    INFEASIBLE,
    LIMIT_REACHED,
    MODEL_REJECTED,
};

struct SolveCertificate {
    bool proven = false;
    int64_t primal_bound = 0;
    int64_t dual_bound = 0;
    int64_t absolute_gap = 0;
    double relative_gap = 0;
    uint64_t explored_nodes = 0;
    std::string scope;
};

struct ReplayEvidence {
    uint32_t requested_unit_actions = 0;
    uint32_t successful_unit_actions = 0;
    uint32_t requested_acquisition_units = 0;
    uint32_t successful_acquisition_units = 0;
    uint32_t failed_hours = 0;
    uint64_t schedule_hash = 0;
    uint64_t end_state_hash = 0;
    bool strict_valid = false;
};

struct DayCandidate {
    std::array<kag::Action, HOURS> actions{};
    std::vector<SaleAvailability> sale_availability;
    std::array<HourSummary, HOURS> hours{};
    std::vector<OutcomeValue> outcomes;
    DayEndState end;
    PhysicalCosts costs;
    ReplayEvidence replay;
    SolveCertificate certificate;
};

struct FailureWitness {
    std::string reason;
    std::vector<OutcomeValue> reachable_bounds;
    std::optional<DayCandidate> diagnostic_candidate;
};

struct DayFrontier {
    uint32_t format_version = FORMAT_VERSION;
    SolveStatus status = SolveStatus::MODEL_REJECTED;
    std::vector<DayCandidate> points;
    std::vector<FailureWitness> failures;
    bool frontier_complete = false;
};

}  // namespace day_solver
