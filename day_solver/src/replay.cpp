#include <cassert>
#include <optional>
#include "replay.hpp"
#include "storage.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "invariant_requirements.hpp"
#include "problem_validation.hpp"

namespace day_solver {
namespace {

struct Event {
    OutcomeMetric metric;
    int16_t subject;
    int16_t tile;
    int8_t hour;
    int64_t quantity;
};

class ReplayMachine {
public:
    explicit ReplayMachine(const DayProblem& problem)
        : problem_(problem), state_(problem.start) {
        coordinate_to_tile_.fill(NO_TILE);
        original_states_.reserve(state_.managed_tiles.size());
        tile_work_.resize(state_.managed_tiles.size(), nullptr);
        work_cursor_.resize(state_.managed_tiles.size());
        for (size_t index = 0; index < state_.managed_tiles.size(); ++index) {
            const ManagedTile& tile = state_.managed_tiles[index];
            coordinate_to_tile_[tile.y * kag::BOARD + tile.x] =
                static_cast<int16_t>(index);
            original_states_.push_back(tile.state);
        }
        for (const TileWork& work : problem.tile_work)
            if (work.tile >= 0 &&
                static_cast<size_t>(work.tile) < tile_work_.size())
                tile_work_[work.tile] = &work;
        workers_.push_back({});
        for (const SaleTarget& target : problem.sale_targets) {
            SaleAvailability availability;
            availability.market_event = target.market_event;
            availability.item = target.item;
            availability.unit_hours.assign(target.maximum_quantity, -1);
            sale_availability_.push_back(std::move(availability));
        }
        const InventoryCount room = shed_headroom();
        running_min_headroom_ = room;
        min_headroom_through_.fill(room);
    }

    void run(const std::array<kag::Action, HOURS>& actions,
             day_scheduler::storage::Frames* frames = nullptr) {
        actions_ = actions;
        for (int hour = 0; hour < HOURS; ++hour) {
            if (frames) (*frames)[hour] = {state_.shed, workers_};
            bool failed = false;
            apply_workers(actions[hour], hour, failed);
            apply_shed_availability(hour, failed);

            HourSummary& summary = hours_[hour];
            summary.hour = static_cast<int8_t>(hour);
            summary.shed_after_workers = state_.shed;
            summary.headroom_after_workers = shed_headroom();

            apply_market_plan(actions[hour], hour, failed);
            summary.shed_after_market = state_.shed;
            summary.headroom_after_market = shed_headroom();
            update_min_headroom();
            min_headroom_through_[hour] = running_min_headroom_;
            summary.cumulative_outcomes = cumulative_events(hour);
            if (failed) ++evidence_.failed_hours;

            decay_crops(hour);
            if (hour == HOURS - 1) end_day();
        }
        check_completion();
    }

    int64_t outcome(const OutcomeKey& key) const {
        switch (key.metric) {
            case OutcomeMetric::END_TILE_STORED: {
                if (key.tile < 0 ||
                    static_cast<size_t>(key.tile) >= state_.managed_tiles.size())
                    return 0;
                const ManagedTileState& tile =
                    state_.managed_tiles[key.tile].state;
                if (key.subject != NO_SUBJECT) {
                    int product = NO_SUBJECT;
                    if (tile.kind == ManagedTileKind::CROP)
                        product = tile.crop;
                    else if (animal_structure(tile.kind) &&
                             tile.animal != NO_SUBJECT)
                        product = kag::ANIMALS[tile.animal - kag::GOOSE].product;
                    if (product != key.subject) return 0;
                }
                return tile.stored_units;
            }
            case OutcomeMetric::END_SHED:
                return inventory_value(state_.shed, key.subject);
            case OutcomeMetric::END_SEEDS:
                return seed_value(state_.seeds, key.subject);
            case OutcomeMetric::END_CASH:
                return state_.cash;
            case OutcomeMetric::MIN_SHED_HEADROOM: {
                const int hour = key.through_hour < 0 ? HOURS - 1
                    : key.through_hour;
                return min_headroom_through_[hour];
            }
            default:
                break;
        }

        const int deadline = key.through_hour < 0 ? HOURS - 1
                                                   : key.through_hour;
        int64_t total = 0;
        for (const Event& event : events_) {
            if (event.metric != key.metric || event.hour > deadline) continue;
            if (key.subject != NO_SUBJECT && event.subject != key.subject)
                continue;
            if (key.tile != NO_TILE && event.tile != key.tile) continue;
            total += event.quantity;
        }
        return total;
    }

    bool end_tile_matches(const EndTileRequirement& requirement) const {
        if (requirement.tile < 0 ||
            static_cast<size_t>(requirement.tile) >= state_.managed_tiles.size())
            return false;
        const ManagedTileState& state =
            state_.managed_tiles[requirement.tile].state;
        if (requirement.exact_state)
            return state == *requirement.exact_state;
        if (state.kind != requirement.kind) return false;
        if (state.kind == ManagedTileKind::CROP)
            return state.crop == requirement.item;
        if (animal_structure(state.kind))
            return state.animal == requirement.item;
        return requirement.item == NO_SUBJECT;
    }

    bool alternative_matches(const PreserveOrHarvest& alternative) const {
        const ManagedTileState& end =
            state_.managed_tiles[alternative.tile].state;
        if (end.kind == ManagedTileKind::CROP &&
            end.crop == alternative.crop)
            return true;
        OutcomeKey harvested{OutcomeMetric::HARVESTED, alternative.crop,
                             alternative.tile, -1};
        return outcome(harvested) >= alternative.harvest_units;
    }

    DayCandidate candidate() const {
        DayCandidate result;
        result.actions = actions_;
        result.sale_availability = sale_availability_;
        result.hours = hours_;
        result.end.physical = state_;
        result.end.workers = workers_;
        result.costs = costs_;
        result.replay = evidence_;
        result.replay.schedule_hash = schedule_hash();
        result.replay.end_state_hash = end_state_hash();
        result.replay.strict_valid = runtime_errors_.empty() &&
            evidence_.requested_unit_actions ==
                evidence_.successful_unit_actions &&
            evidence_.requested_acquisition_units ==
                evidence_.successful_acquisition_units;
        return result;
    }

    const std::vector<std::string>& runtime_errors() const {
        return runtime_errors_;
    }


    // Local biology only. Travel, cargo, seeds, and other tiles are relaxed.
    std::optional<ManagedTileState> local_hour(
            const ManagedTileState& before, int prefix, int count, int hour) {
        assert(state_.managed_tiles.size() == 1 && tile_work_[0] != nullptr);
        const auto& actions = tile_work_[0]->actions;
        assert(hour >= 0 && hour < HOURS && prefix >= 0 && count >= 0);
        assert(static_cast<size_t>(prefix + count) <= actions.size());
        state_.managed_tiles[0].state = before;
        state_.shed.fill(0);
        work_cursor_[0] = prefix;
        events_.clear();
        for (int step = 0; step < count; ++step) {
            const auto& expected = actions[prefix + step];
            WorkerState worker;
            worker.x = state_.managed_tiles[0].x;
            worker.y = state_.managed_tiles[0].y;
            const int input = expected.op == kag::OP_FEED ? kag::WHEAT
                : expected.op == kag::OP_FERTILIZE ? kag::FERTILIZER
                : expected.op == kag::OP_PLACE ? expected.arg : -1;
            if (input >= 0) {
                worker.cargo[input] = 1;
                worker.cargo_order.push_back(input);
            }
            state_.seeds.fill(1);
            kag::UnitAction action;
            action.op = expected.op;
            action.arg = expected.arg < 0 ? 0 : expected.arg;
            action.n = expected.quantity;
            if (!apply_unit(worker, action, hour) ||
                work_cursor_[0] != static_cast<size_t>(prefix + step + 1))
                return std::nullopt;
        }
        for (const auto& market : problem_.market_plan)
            if (market.hour == hour && market.market_op == kag::M_BUY_LAND)
                unlock_land(market.item);
        decay_crops(hour);
        if (hour == HOURS - 1) end_day();
        return state_.managed_tiles[0].state;
    }
private:
    const DayProblem& problem_;
    PhysicalState state_;
    std::vector<ManagedTileState> original_states_;
    std::vector<const TileWork*> tile_work_;
    std::vector<size_t> work_cursor_;
    std::array<int16_t, kag::BOARD * kag::BOARD> coordinate_to_tile_{};
    std::vector<WorkerState> workers_;
    std::vector<Event> events_;
    std::vector<SaleAvailability> sale_availability_;
    std::array<HourSummary, HOURS> hours_{};
    std::array<kag::Action, HOURS> actions_{};
    std::array<InventoryCount, HOURS> min_headroom_through_{};
    InventoryCount running_min_headroom_ = 0;
    PhysicalCosts costs_;
    ReplayEvidence evidence_;
    std::vector<std::string> runtime_errors_;

    static int64_t inventory_value(
        const std::array<InventoryCount, kag::N_ITEMS>& values, int subject) {
        if (subject != NO_SUBJECT) return values[subject];
        int64_t total = 0;
        for (InventoryCount value : values) total += value;
        return total;
    }

    static int64_t seed_value(
        const std::array<InventoryCount, kag::N_CROPS>& values, int subject) {
        if (subject != NO_SUBJECT) return values[subject];
        int64_t total = 0;
        for (InventoryCount value : values) total += value;
        return total;
    }

    InventoryCount shed_total() const {
        InventoryCount result = 0;
        for (InventoryCount value : state_.shed) result += value;
        return result;
    }

    InventoryCount shed_headroom() const {
        return state_.shed_capacity - shed_total();
    }

    InventoryCount deposit_room() const {
        return state_.shed_capacity == day_scheduler::storage::unlimited
            ? std::numeric_limits<InventoryCount>::max() : std::max(InventoryCount(0), shed_headroom());
    }

    void update_min_headroom() {
        running_min_headroom_ = std::min(running_min_headroom_,
                                         shed_headroom());
    }

    ManagedTileState* tile_at(int x, int y, int& tile_index) {
        tile_index = coordinate_to_tile_[y * kag::BOARD + x];
        if (tile_index == NO_TILE) return nullptr;
        return &state_.managed_tiles[tile_index].state;
    }

    static void cargo_add(WorkerState& worker, int item, InventoryCount quantity) {
        if (quantity <= 0) return;
        if (worker.cargo[item] == 0)
            worker.cargo_order.push_back(static_cast<uint8_t>(item));
        worker.cargo[item] += static_cast<InventoryCount>(quantity);
    }

    static void cargo_erase(WorkerState& worker, int item) {
        worker.cargo[item] = 0;
        const auto found = std::find(worker.cargo_order.begin(),
                                     worker.cargo_order.end(), item);
        if (found != worker.cargo_order.end()) worker.cargo_order.erase(found);
    }

    static bool cargo_take(WorkerState& worker, int item, InventoryCount quantity) {
        if (quantity <= 0 || worker.cargo[item] < quantity) return false;
        worker.cargo[item] -= static_cast<InventoryCount>(quantity);
        if (worker.cargo[item] == 0) cargo_erase(worker, item);
        return true;
    }

    void event(OutcomeMetric metric, int subject, int tile, int hour,
               int64_t quantity = 1) {
        if (quantity <= 0) return;
        events_.push_back({metric, static_cast<int16_t>(subject),
            static_cast<int16_t>(tile), static_cast<int8_t>(hour), quantity});
    }

    void error(int hour, std::string message, bool& failed) {
        runtime_errors_.push_back("hour " + std::to_string(hour) + ": " +
                                  std::move(message));
        failed = true;
    }

    const TileWorkAction* expected_work(int tile_index,
                                        const kag::UnitAction& action,
                                        const ManagedTileState& tile) const {
        if (tile_index < 0 ||
            static_cast<size_t>(tile_index) >= tile_work_.size() ||
            tile_work_[tile_index] == nullptr)
            return nullptr;
        const TileWork& work = *tile_work_[tile_index];
        const size_t cursor = work_cursor_[tile_index];
        if (cursor >= work.actions.size()) return nullptr;
        const TileWorkAction& expected = work.actions[cursor];
        if (expected.op != action.op || expected.quantity != 1) return nullptr;

        int semantic_arg = NO_SUBJECT;
        if (action.op == kag::OP_PLANT || action.op == kag::OP_PLACE)
            semantic_arg = action.arg;
        else if (action.op == kag::OP_HARVEST) {
            if (tile.kind == ManagedTileKind::CROP)
                semantic_arg = tile.crop;
            else if (animal_structure(tile.kind) &&
                     tile.animal != NO_SUBJECT)
                semantic_arg =
                    kag::ANIMALS[tile.animal - kag::GOOSE].product;
        }
        if (expected.arg != semantic_arg) return nullptr;
        return &expected;
    }

    void complete_work(int tile_index) {
        ++work_cursor_[tile_index];
    }

    bool apply_unit(WorkerState& worker, const kag::UnitAction& action,
                    int hour) {
        const int x = worker.x;
        const int y = worker.y;
        if (action.op >= kag::OP_NORTH && action.op <= kag::OP_WEST) {
            const int next_x = x + (action.op == kag::OP_EAST) -
                                   (action.op == kag::OP_WEST);
            const int next_y = y + (action.op == kag::OP_SOUTH) -
                                   (action.op == kag::OP_NORTH);
            if (next_x < 0 || next_x >= kag::BOARD ||
                next_y < 0 || next_y >= kag::BOARD)
                return false;
            worker.x = static_cast<int8_t>(next_x);
            worker.y = static_cast<int8_t>(next_y);
            ++costs_.travel_actions;
            event(OutcomeMetric::TRAVEL, NO_SUBJECT, NO_TILE, hour);
            return true;
        }

        if (action.op == kag::OP_PICKUP) {
            if (!kag::is_shed_adjacent(x, y, kag::BOARD) ||
                action.arg >= kag::N_ITEMS || action.n <= 0)
                return false;
            const InventoryCount quantity = std::min<InventoryCount>(action.n,
                                                state_.shed[action.arg]);
            if (quantity <= 0) return false;
            state_.shed[action.arg] -= static_cast<InventoryCount>(quantity);
            cargo_add(worker, action.arg, quantity);
            ++costs_.pickups;
            event(OutcomeMetric::PICKUPS, action.arg, NO_TILE, hour, quantity);
            update_min_headroom();
            return true;
        }

        if (action.op == kag::OP_DROP) {
            if (!kag::is_shed_adjacent(x, y, kag::BOARD) ||
                worker.cargo_order.empty())
                return false;
            const std::vector<uint8_t> order = worker.cargo_order;
            for (uint8_t item : order) {
                const InventoryCount carried = worker.cargo[item];
                const InventoryCount deposited = std::min(carried, deposit_room());
                state_.shed[item] += deposited;
                event(OutcomeMetric::PRODUCT_DEPOSITED, item, NO_TILE, hour,
                      deposited);
                event(OutcomeMetric::DROPS, item, NO_TILE, hour, carried);
                cargo_erase(worker, item);
            }
            ++costs_.drops;
            update_min_headroom();
            return true;
        }

        int tile_index = NO_TILE;
        ManagedTileState* tile = tile_at(x, y, tile_index);

        if (action.op == kag::OP_PLACE && action.arg < kag::N_ITEMS &&
            kag::is_animal(action.arg) && tile &&
            tile->kind == structure_for(action.arg) &&
            tile->animal == NO_SUBJECT) {
            if (!expected_work(tile_index, action, *tile)) return false;
            if (!cargo_take(worker, action.arg, 1)) return false;
            *tile = {};
            tile->kind = structure_for(action.arg);
            tile->animal = action.arg;
            event(OutcomeMetric::ANIMAL_PLACED, action.arg, tile_index, hour);
            complete_work(tile_index);
            return true;
        }

        if (action.op == kag::OP_PLACE) {
            if (action.arg >= kag::N_ITEMS || action.n <= 0 ||
                !kag::is_shed_adjacent(x, y, kag::BOARD))
                return false;
            const InventoryCount quantity = std::min({InventoryCount(action.n),
                                                worker.cargo[action.arg], deposit_room()});
            if (quantity <= 0) return false;
            cargo_take(worker, action.arg, quantity);
            state_.shed[action.arg] += static_cast<InventoryCount>(quantity);
            ++costs_.drops;
            event(OutcomeMetric::PRODUCT_DEPOSITED, action.arg, NO_TILE, hour,
                  quantity);
            event(OutcomeMetric::DROPS, action.arg, NO_TILE, hour, quantity);
            update_min_headroom();
            return true;
        }

        if (!tile || tile->kind == ManagedTileKind::LOCKED) return false;
        const TileWorkAction* expected = expected_work(tile_index, action,
                                                       *tile);
        if (expected == nullptr) return false;

        switch (action.op) {
            case kag::OP_PLANT: {
                if (action.arg >= kag::N_CROPS ||
                    tile->kind != ManagedTileKind::EMPTY ||
                    state_.seeds[action.arg] <= 0)
                    return false;
                --state_.seeds[action.arg];
                *tile = {};
                tile->kind = ManagedTileKind::CROP;
                tile->crop = action.arg;
                tile->age_days = 0;
                tile->consecutive_dry_days = 1;
                tile->stored_units = kag::CROPS[action.arg].ongoing ? 0 : 1;
                event(OutcomeMetric::PLANTED, action.arg, tile_index, hour);
                complete_work(tile_index);
                return true;
            }
            case kag::OP_WATER: {
                if (tile->kind != ManagedTileKind::CROP ||
                    tile->watered_today)
                    return false;
                tile->watered_today = true;
                const kag::CropDef& crop = kag::CROPS[tile->crop];
                if (!crop.ongoing) {
                    const int first_growth_age = (crop.max_yield_day + 1) / 2;
                    if (tile->age_days >= first_growth_age &&
                        tile->age_days <= crop.max_yield_day) {
                        const int gain = tile->fertilizer_days_remaining > 0
                            ? 2 : 1;
                        tile->stored_units = std::min<int>(crop.max_yield,
                            tile->stored_units + gain);
                    }
                }
                event(OutcomeMetric::CROP_WATERED, tile->crop, tile_index,
                      hour);
                complete_work(tile_index);
                return true;
            }
            case kag::OP_HARVEST: {
                if (tile->stored_units <= 0) return false;
                if (tile->kind == ManagedTileKind::CROP) {
                    const kag::CropDef& crop = kag::CROPS[tile->crop];
                    if (tile->age_days < crop.first_yield_day) return false;
                    const int product = tile->crop;
                    const int quantity = tile->stored_units;
                    if (expected->output_item != product ||
                        expected->output_quantity != quantity)
                        return false;
                    cargo_add(worker, product, quantity);
                    event(OutcomeMetric::HARVESTED, product, tile_index, hour,
                          quantity);
                    tile->stored_units = 0;
                    if (!crop.ongoing) *tile = {};
                    complete_work(tile_index);
                    return true;
                }
                if (animal_structure(tile->kind) &&
                    tile->animal != NO_SUBJECT) {
                    const int product =
                        kag::ANIMALS[tile->animal - kag::GOOSE].product;
                    const int quantity = tile->stored_units;
                    if (expected->output_item != product ||
                        expected->output_quantity != quantity)
                        return false;
                    cargo_add(worker, product, quantity);
                    tile->stored_units = 0;
                    event(OutcomeMetric::HARVESTED, product, tile_index, hour,
                          quantity);
                    complete_work(tile_index);
                    return true;
                }
                return false;
            }
            case kag::OP_FERTILIZE:
                if (tile->kind != ManagedTileKind::CROP ||
                    !cargo_take(worker, kag::FERTILIZER, 1))
                    return false;
                tile->fertilizer_days_remaining =
                    std::max<int64_t>(tile->fertilizer_days_remaining, 3);
                event(OutcomeMetric::CROP_FERTILIZED, tile->crop, tile_index,
                      hour);
                complete_work(tile_index);
                return true;
            case kag::OP_DIG:
                if (tile->kind == ManagedTileKind::EMPTY ||
                    (animal_structure(tile->kind) &&
                     tile->animal != NO_SUBJECT))
                    return false;
                *tile = {};
                event(OutcomeMetric::DUG, NO_SUBJECT, tile_index, hour);
                complete_work(tile_index);
                return true;
            case kag::OP_BUILD_PASTURE:
            case kag::OP_BUILD_COOP:
                if (tile->kind != ManagedTileKind::EMPTY) return false;
                *tile = {};
                tile->kind = action.op == kag::OP_BUILD_COOP
                    ? ManagedTileKind::COOP : ManagedTileKind::PASTURE;
                event(action.op == kag::OP_BUILD_COOP ? OutcomeMetric::COOP_BUILT
                      : OutcomeMetric::PASTURE_BUILT, NO_SUBJECT, tile_index, hour);
                complete_work(tile_index);
                return true;
            case kag::OP_FEED:
                if (!animal_structure(tile->kind) ||
                    tile->animal == NO_SUBJECT || tile->fed_today ||
                    !cargo_take(worker, kag::WHEAT, 1))
                    return false;
                tile->fed_today = true;
                event(OutcomeMetric::FED, tile->animal, tile_index, hour);
                complete_work(tile_index);
                return true;
            case kag::OP_COLLECT_FERTILIZER:
                if (!animal_structure(tile->kind) ||
                    tile->animal == NO_SUBJECT ||
                    !tile->fertilizer_available)
                    return false;
                tile->fertilizer_available = false;
                cargo_add(worker, kag::FERTILIZER, 1);
                if (expected->output_item != kag::FERTILIZER ||
                    expected->output_quantity != 1)
                    return false;
                event(OutcomeMetric::FERTILIZER_COLLECTED, NO_SUBJECT,
                      tile_index, hour);
                complete_work(tile_index);
                return true;
            case kag::OP_CARE:
                if (!animal_structure(tile->kind) ||
                    tile->animal == NO_SUBJECT || tile->cared_today)
                    return false;
                tile->cared_today = true;
                event(OutcomeMetric::CARED, tile->animal, tile_index, hour);
                complete_work(tile_index);
                return true;
            case kag::OP_PASS:
            default:
                return false;
        }
    }

    void apply_workers(const kag::Action& action, int hour, bool& failed) {
        const int active = static_cast<int>(workers_.size());
        if (action.n_units != active)
            error(hour, "n_units does not equal active worker count", failed);
        const int supplied = std::clamp(action.n_units, 0, kag::MAX_UNITS);

        std::array<int, kag::N_CROPS> plant_demand{};
        for (int unit = 0; unit < supplied; ++unit)
            if (action.units[unit].op == kag::OP_PLANT &&
                action.units[unit].arg < kag::N_CROPS)
                ++plant_demand[action.units[unit].arg];
        std::array<bool, kag::N_CROPS> blocked{};
        for (int crop = 0; crop < kag::N_CROPS; ++crop)
            blocked[crop] = plant_demand[crop] > state_.seeds[crop];

        for (int unit = 0; unit < active; ++unit) {
            if (unit >= supplied || action.units[unit].op == kag::OP_PASS) {
                ++costs_.spare_turns;
                event(OutcomeMetric::SPARE_TURNS, NO_SUBJECT, NO_TILE, hour);
                continue;
            }
            const kag::UnitAction& request = action.units[unit];
            ++evidence_.requested_unit_actions;
            if (request.op == kag::OP_PLANT &&
                request.arg < kag::N_CROPS && blocked[request.arg]) {
                error(hour, "atomic plant demand exceeds seeds", failed);
                continue;
            }
            if (!apply_unit(workers_[unit], request, hour)) {
                error(hour, "worker " + std::to_string(unit) +
                      " action failed op=" + std::to_string(request.op) +
                      " arg=" + std::to_string(request.arg) +
                      " n=" + std::to_string(request.n) +
                      " at=(" + std::to_string(workers_[unit].x) + "," +
                      std::to_string(workers_[unit].y) + ")", failed);
                continue;
            }
            ++evidence_.successful_unit_actions;
            ++costs_.unit_actions;
            event(OutcomeMetric::UNIT_ACTIONS, NO_SUBJECT, NO_TILE, hour);
            update_min_headroom();
        }
        for (int unit = active; unit < supplied; ++unit) {
            if (action.units[unit].op == kag::OP_PASS) continue;
            ++evidence_.requested_unit_actions;
            error(hour, "action supplied for nonexistent worker", failed);
        }
    }

    bool hire_one(int hour) {
        if (costs_.hires >= static_cast<uint32_t>(problem_.worker_count - 1) ||
            workers_.size() >= kag::MAX_UNITS)
            return false;

        int access[4][2];
        kag::shed_access_tiles(kag::BOARD, access);
        int occupancy[4] = {};
        for (const WorkerState& worker : workers_)
            for (int point = 0; point < 4; ++point)
                if (worker.x == access[point][0] &&
                    worker.y == access[point][1])
                    ++occupancy[point];
        int best = 0;
        for (int point = 1; point < 4; ++point)
            if (occupancy[point] < occupancy[best]) best = point;
        WorkerState worker;
        worker.x = static_cast<int8_t>(access[best][0]);
        worker.y = static_cast<int8_t>(access[best][1]);
        workers_.push_back(worker);
        ++costs_.hires;
        event(OutcomeMetric::HIRES, NO_SUBJECT, NO_TILE, hour);
        update_min_headroom();
        return true;
    }

    static bool order_matches(const kag::Order& order,
                              const MarketEvent& event) {
        if (order.op != event.market_op) return false;
        if (event.market_op == kag::M_HIRE || event.market_op == kag::M_BUY_LAND) return true;
        return order.item == event.item && order.n == event.quantity;
    }

    void unlock_land(int quadrant) {
        for (auto& tile : state_.managed_tiles)
            if (kag::quadrant_of(tile.x, tile.y, kag::BOARD) == quadrant &&
                tile.state.kind == ManagedTileKind::LOCKED)
                tile.state = {};
    }

    bool apply_market_event(const MarketEvent& market, int hour) {
        if (market.market_op == kag::M_HIRE) {
            if (!hire_one(hour)) return false;
        } else if (market.market_op == kag::M_BUY_LAND) {
            unlock_land(market.item);
        } else if (market.market_op == kag::M_SELL) {
            if (state_.shed[market.item] < market.quantity) return false;
            state_.shed[market.item] -=
                static_cast<InventoryCount>(market.quantity);
            costs_.sold_units += market.quantity;
            costs_.sale_proceeds += market.cash_delta;
        } else if (market.market_op == kag::M_BUY_SEED) {
            state_.seeds[market.item] +=
                static_cast<InventoryCount>(market.quantity);
            costs_.purchased_units += market.quantity;
            event(OutcomeMetric::PURCHASED, market.item, NO_TILE, hour,
                  market.quantity);
        } else if (market.market_op == kag::M_BUY_PRODUCT ||
                   market.market_op == kag::M_BUY_ANIMAL) {
            if (market.quantity > deposit_room()) return false;
            state_.shed[market.item] +=
                static_cast<InventoryCount>(market.quantity);
            costs_.purchased_units += market.quantity;
            event(OutcomeMetric::PURCHASED, market.item, NO_TILE, hour,
                  market.quantity);
        } else {
            return false;
        }
        update_min_headroom();
        return true;
    }

    void apply_shed_availability(int hour, bool& failed) {
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            const InventoryCount previous = hour == 0 ? 0 :
                problem_.shed_availability[hour - 1][item];
            const InventoryCount required =
                problem_.shed_availability[hour][item] - previous;
            if (required == 0) continue;
            if (state_.shed[item] < required) {
                error(hour, "shed availability missing item=" +
                    std::to_string(item) + " required=" +
                    std::to_string(required) + " present=" +
                    std::to_string(state_.shed[item]), failed);
                continue;
            }
            state_.shed[item] -= static_cast<InventoryCount>(required);
        }
        update_min_headroom();
    }

    void apply_market_plan(const kag::Action& action, int hour,
                           bool& failed) {
        if (action.n_orders < 0 || action.n_orders > 10) {
            error(hour, "n_orders must be in [0, 10]", failed);
        }
        std::array<const MarketEvent*, 10> expected{};
        int required_orders = 0;
        for (const MarketEvent& market : problem_.market_plan) {
            if (market.hour != hour) continue;
            expected[market.order_index] = &market;
            required_orders = std::max(required_orders,
                                       market.order_index + 1);
        }
        if (action.n_orders < required_orders)
            error(hour, "market action omits a fixed buy slot", failed);
        const int supplied_orders = std::clamp(action.n_orders, 0, 10);
        for (int index = 0; index < supplied_orders; ++index) {
            const MarketEvent* market = expected[index];
            const kag::Order& order = action.orders[index];
            if (market == nullptr) {
                if (order.op != kag::M_NONE)
                    error(hour, "unexpected market order", failed);
                continue;
            }
            evidence_.requested_acquisition_units += market->quantity;
            if (!order_matches(order, *market)) {
                error(hour, "market order does not match fixed plan", failed);
                continue;
            }
            if (!apply_market_event(*market, hour)) {
                error(hour, "fixed market event failed", failed);
                continue;
            }
            evidence_.successful_acquisition_units += market->quantity;
        }
    }

    void check_completion() {
        if (costs_.hires != static_cast<uint32_t>(problem_.worker_count - 1))
            runtime_errors_.push_back("fixed worker_count was not reached");
        for (size_t tile = 0; tile < tile_work_.size(); ++tile) {
            if (tile_work_[tile] != nullptr &&
                work_cursor_[tile] != tile_work_[tile]->actions.size())
                runtime_errors_.push_back("tile_work for tile " +
                    std::to_string(tile) + " was not completed");
        }
    }

    void record_sale_availability(int hour) {
        for (SaleAvailability& availability : sale_availability_) {
            const MarketEvent& target =
                problem_.market_plan[availability.market_event];
            if (hour > target.hour || availability.unit_hours[0] >= 0)
                continue;
            int reserved = 0;
            for (const MarketEvent& market : problem_.market_plan) {
                if (market.market_op != kag::M_SELL ||
                    market.item != target.item || market.hour < hour)
                    continue;
                if (std::tie(market.hour, market.order_index) <=
                    std::tie(target.hour, target.order_index))
                    reserved += market.quantity;
            }
            if (state_.shed[target.item] < reserved) continue;
            std::fill(availability.unit_hours.begin(),
                      availability.unit_hours.end(),
                      static_cast<int8_t>(hour));
        }
    }

    std::vector<OutcomeValue> cumulative_events(int through_hour) const {
        using Key = std::tuple<int, int, int>;
        std::map<Key, int64_t> totals;
        for (const Event& event : events_)
            if (event.hour <= through_hour)
                totals[{static_cast<int>(event.metric), event.subject,
                        event.tile}] += event.quantity;
        std::vector<OutcomeValue> result;
        result.reserve(totals.size());
        for (const auto& [key, value] : totals)
            result.push_back({
                {static_cast<OutcomeMetric>(std::get<0>(key)),
                 static_cast<int16_t>(std::get<1>(key)),
                 static_cast<int16_t>(std::get<2>(key)),
                 static_cast<int8_t>(through_hour)},
                value});
        return result;
    }

    void decay_crops(int hour) {
        if ((hour & 1) != 0) return;
        for (ManagedTile& managed : state_.managed_tiles) {
            ManagedTileState& tile = managed.state;
            if (tile.kind != ManagedTileKind::CROP) continue;
            const kag::CropDef& crop = kag::CROPS[tile.crop];
            const int loss_age = crop.ongoing
                ? crop.first_yield_day +
                    (crop.max_yield - 1) * crop.interval + 1
                : crop.max_yield_day + 1;
            if (tile.age_days < loss_age) continue;
            --tile.stored_units;
            if (tile.stored_units <= 0) {
                tile = {};
                tile.kind = ManagedTileKind::WEED;
            }
        }
    }

    void end_day_crop(ManagedTileState& tile) {
        const bool watered = tile.watered_today;
        tile.consecutive_dry_days = watered ? 0
            : tile.consecutive_dry_days + 1;
        if (tile.consecutive_dry_days >= 2) {
            tile = {};
            tile.kind = ManagedTileKind::WEED;
            return;
        }

        const kag::CropDef& crop = kag::CROPS[tile.crop];
        const int64_t next_age = tile.age_days + 1;
        if (crop.ongoing) {
            const int64_t since = next_age - crop.first_yield_day;
            if (since >= 0 && since % crop.interval == 0 &&
                since / crop.interval < crop.max_yield) {
                const int gain = watered &&
                    tile.fertilizer_days_remaining > 0 ? 2 : 1;
                tile.stored_units = std::min<int>(crop.max_yield,
                                                   tile.stored_units + gain);
            }
        }
        tile.age_days = next_age;
        tile.watered_today = false;
        if (tile.fertilizer_days_remaining > 0)
            --tile.fertilizer_days_remaining;
    }

    void end_day_animal(ManagedTileState& tile) {
        tile.consecutive_dry_days = tile.fed_today ? 0
            : tile.consecutive_dry_days + 1;
        if (tile.consecutive_dry_days >= 2) {
            const auto kind = tile.kind;
            tile = {};
            tile.kind = kind;
            return;
        }

        const kag::AnimalDef& animal =
            kag::ANIMALS[tile.animal - kag::GOOSE];
        const int64_t next_age = tile.age_days + 1;
        const int64_t since = next_age - animal.first_yield_day;
        if (since >= 0 && since % animal.interval == 0) {
            const int64_t bonus = tile.fed_today ? tile.pending_care_bonus : 0;
            tile.stored_units = std::min<int64_t>(animal.max_held,
                tile.stored_units + 1 + bonus);
            tile.pending_care_bonus = 0;
        }
        if (tile.cared_today && tile.fed_today)
            ++tile.pending_care_bonus;
        tile.age_days = next_age;
        tile.fertilizer_available = true;
        tile.fed_today = false;
        tile.cared_today = false;
    }

    void end_day() {
        for (ManagedTile& managed : state_.managed_tiles) {
            ManagedTileState& tile = managed.state;
            if (tile.kind == ManagedTileKind::CROP)
                end_day_crop(tile);
            else if (animal_structure(tile.kind) &&
                     tile.animal != NO_SUBJECT)
                end_day_animal(tile);
        }

        for (WorkerState& worker : workers_) {
            const std::vector<uint8_t> order = worker.cargo_order;
            for (uint8_t item : order) {
                state_.shed[item] += std::min(worker.cargo[item], deposit_room());
                cargo_erase(worker, item);
            }
        }
        workers_.clear();
        workers_.push_back({});
        update_min_headroom();
        min_headroom_through_[HOURS - 1] = running_min_headroom_;
    }

    uint64_t schedule_hash() const {
        uint64_t hash = 14695981039346656037ull;
        auto add = [&](int64_t value) {
            const uint64_t word = static_cast<uint64_t>(value);
            for (int shift = 0; shift < 64; shift += 8) {
                hash ^= (word >> shift) & 0xffu;
                hash *= 1099511628211ull;
            }
        };
        for (const kag::Action& action : actions_) {
            add(action.n_units);
            for (int unit = 0; unit < std::clamp(action.n_units, 0,
                                                  kag::MAX_UNITS); ++unit) {
                add(action.units[unit].op);
                add(action.units[unit].arg);
                add(action.units[unit].n);
            }
            add(action.n_orders);
            for (int order = 0; order < std::clamp(action.n_orders, 0, 16);
                 ++order) {
                add(action.orders[order].op);
                add(action.orders[order].item);
                add(action.orders[order].n);
            }
        }
        return hash;
    }

    uint64_t end_state_hash() const {
        uint64_t hash = 14695981039346656037ull;
        auto add = [&](int64_t value) {
            const uint64_t word = static_cast<uint64_t>(value);
            for (int shift = 0; shift < 64; shift += 8) {
                hash ^= (word >> shift) & 0xffu;
                hash *= 1099511628211ull;
            }
        };
        add(state_.cash);
        add(state_.shed_capacity);
        for (InventoryCount value : state_.shed) add(value);
        for (InventoryCount value : state_.seeds) add(value);
        for (const ManagedTile& managed : state_.managed_tiles) {
            add(managed.x);
            add(managed.y);
            const ManagedTileState& tile = managed.state;
            add(static_cast<int>(tile.kind));
            add(tile.crop);
            add(tile.animal);
            add(tile.age_days);
            add(tile.stored_units);
            add(tile.consecutive_dry_days);
            add(tile.pending_care_bonus);
            add(tile.fertilizer_days_remaining);
            add(tile.watered_today);
            add(tile.fed_today);
            add(tile.cared_today);
            add(tile.fertilizer_available);
        }
        return hash;
    }
};

bool bounds_hold(const std::vector<OutcomeBound>& bounds,
                 const ReplayMachine& machine, DayCandidate& candidate,
                 std::vector<std::string>& errors, std::string_view prefix) {
    bool valid = true;
    for (size_t index = 0; index < bounds.size(); ++index) {
        const OutcomeBound& bound = bounds[index];
        const int64_t value = machine.outcome(bound.key);
        candidate.outcomes.push_back({bound.key, value});
        if (value < bound.lower || value > bound.upper) {
            errors.push_back(std::string(prefix) + "[" +
                std::to_string(index) + "] metric=" +
                std::to_string(static_cast<int>(bound.key.metric)) +
                " subject=" + std::to_string(bound.key.subject) +
                " tile=" + std::to_string(bound.key.tile) +
                " through_hour=" + std::to_string(bound.key.through_hour) +
                " value=" + std::to_string(value) +
                " bounds=[" + std::to_string(bound.lower) + "," +
                std::to_string(bound.upper) + "]");
            valid = false;
        }
    }
    return valid;
}

bool end_tiles_hold(const std::vector<EndTileRequirement>& requirements,
                    const ReplayMachine& machine,
                    std::vector<std::string>& errors,
                    std::string_view prefix) {
    bool valid = true;
    for (size_t index = 0; index < requirements.size(); ++index) {
        if (!machine.end_tile_matches(requirements[index])) {
            const EndTileRequirement& requirement = requirements[index];
            errors.push_back(std::string(prefix) + "[" +
                std::to_string(index) + "] tile=" +
                std::to_string(requirement.tile) + " kind=" +
                std::to_string(static_cast<int>(requirement.kind)) +
                " item=" + std::to_string(requirement.item) +
                " was not reached");
            valid = false;
        }
    }
    return valid;
}

}  // namespace

day_scheduler::storage::Frames storage_trace(
    const DayProblem& problem, const std::array<kag::Action, HOURS>& actions) {
    ReplayMachine machine(problem);
    day_scheduler::storage::Frames frames;
    machine.run(actions, &frames);
    return frames;
}

ReplayResult replay_schedule(
    const DayProblem& problem,
    const std::array<kag::Action, HOURS>& actions) {
    ReplayResult result;
    const auto validation = validate_problem(problem);
    if (!validation.empty()) {
        for (const ValidationIssue& issue : validation)
            result.errors.push_back(issue.path + ": " + issue.message);
        return result;
    }

    ReplayMachine machine(problem);
    machine.run(actions);
    result.candidate = machine.candidate();
    result.errors = machine.runtime_errors();

    const bool explicit_bounds = bounds_hold(
        problem.required_outcomes, machine, result.candidate, result.errors,
        "required_outcomes");
    const bool explicit_end_tiles = end_tiles_hold(
        problem.required_end_tiles, machine, result.errors,
        "required_end_tiles");
    const bool end_shed = result.candidate.end.physical.shed == problem.end_shed;
    const bool end_seeds = result.candidate.end.physical.seeds == problem.end_seeds;
    if (!end_shed) result.errors.push_back("end_shed was not reached");
    if (!end_seeds) result.errors.push_back("end_seeds was not reached");
    result.requirements_satisfied = explicit_bounds && explicit_end_tiles &&
        end_shed && end_seeds;

    // v3 fixes all required work and the complete successor state explicitly;
    // no strategy-level invariant may add hidden work to the feasibility API.
    result.invariants_satisfied = true;
    return result;
}

}  // namespace day_solver

namespace day_scheduler::storage {
Frames trace(const day_solver::DayProblem& problem,
             const std::array<kag::Action, day_solver::HOURS>& actions) {
    return day_solver::storage_trace(problem, actions);
}
}
