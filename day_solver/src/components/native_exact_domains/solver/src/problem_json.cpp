#include "problem_json.hpp"

#include <boost/json.hpp>
#include <boost/json/src.hpp>

#include <array>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "problem_validation.hpp"

namespace day_solver {
namespace {

namespace json = boost::json;

const json::object& object(const json::value& value, std::string_view path) {
    if (!value.is_object())
        throw std::runtime_error(std::string(path) + " must be an object");
    return value.as_object();
}

const json::array& array(const json::value& value, std::string_view path) {
    if (!value.is_array())
        throw std::runtime_error(std::string(path) + " must be an array");
    return value.as_array();
}

void exact_keys(const json::object& value,
                std::initializer_list<std::string_view> expected,
                std::string_view path) {
    for (const auto& member : value) {
        bool found = false;
        for (std::string_view key : expected) found |= member.key() == key;
        if (!found)
            throw std::runtime_error(std::string(path) +
                " has unknown field " + std::string(member.key()));
    }
    for (std::string_view key : expected)
        if (!value.contains(key))
            throw std::runtime_error(std::string(path) +
                " is missing field " + std::string(key));
}

int64_t integer(const json::value& value, std::string_view path) {
    if (value.is_int64()) return value.as_int64();
    if (value.is_uint64() &&
        value.as_uint64() <= static_cast<uint64_t>(
            std::numeric_limits<int64_t>::max()))
        return static_cast<int64_t>(value.as_uint64());
    throw std::runtime_error(std::string(path) + " must be an integer");
}

uint64_t unsigned_integer(const json::value& value, std::string_view path) {
    if (value.is_uint64()) return value.as_uint64();
    if (value.is_int64() && value.as_int64() >= 0)
        return static_cast<uint64_t>(value.as_int64());
    throw std::runtime_error(std::string(path) +
        " must be a nonnegative integer");
}

bool boolean(const json::value& value, std::string_view path) {
    if (!value.is_bool())
        throw std::runtime_error(std::string(path) + " must be a boolean");
    return value.as_bool();
}

std::string string(const json::value& value, std::string_view path) {
    if (!value.is_string())
        throw std::runtime_error(std::string(path) + " must be a string");
    return std::string(value.as_string());
}

template <class T>
T narrow(int64_t value, std::string_view path) {
    if (value < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
        value > static_cast<int64_t>(std::numeric_limits<T>::max()))
        throw std::runtime_error(std::string(path) + " is out of range");
    return static_cast<T>(value);
}

template <size_t N>
std::array<InventoryCount, N> read_inventory(const json::value& value,
                                  const std::string& path) {
    const auto& input = array(value, path);
    if (input.size() != N)
        throw std::runtime_error(path + " has the wrong length");
    std::array<InventoryCount, N> result{};
    for (size_t index = 0; index < N; ++index) {
        const std::string item_path = path + "[" + std::to_string(index) + "]";
        result[index] = narrow<int32_t>(integer(input[index], item_path), item_path);
    }
    return result;
}

template <class T, size_t N>
json::array write_fixed_array(const std::array<T, N>& values) {
    json::array result;
    result.reserve(N);
    for (T value : values) result.push_back(static_cast<int64_t>(value));
    return result;
}

const char* kind_name(ManagedTileKind kind) {
    switch (kind) {
        case ManagedTileKind::EMPTY: return "empty";
        case ManagedTileKind::WEED: return "weed";
        case ManagedTileKind::PASTURE: return "pasture";
        case ManagedTileKind::CROP: return "crop";
        case ManagedTileKind::COOP: return "coop";
        case ManagedTileKind::LOCKED: return "locked";
    }
    throw std::runtime_error("invalid managed tile kind");
}

ManagedTileKind parse_kind(const json::value& value, std::string_view path) {
    const std::string name = string(value, path);
    for (int raw = static_cast<int>(ManagedTileKind::EMPTY);
         raw <= static_cast<int>(ManagedTileKind::LOCKED); ++raw) {
        const auto kind = static_cast<ManagedTileKind>(raw);
        if (name == kind_name(kind)) return kind;
    }
    throw std::runtime_error(std::string(path) + " has unknown kind " + name);
}

const char* metric_name(OutcomeMetric metric) {
    switch (metric) {
        case OutcomeMetric::PLANTED: return "planted";
        case OutcomeMetric::HARVESTED: return "harvested";
        case OutcomeMetric::ANIMAL_PLACED: return "animal_placed";
        case OutcomeMetric::FED: return "fed";
        case OutcomeMetric::CARED: return "cared";
        case OutcomeMetric::FERTILIZER_COLLECTED: return "fertilizer_collected";
        case OutcomeMetric::CROP_WATERED: return "crop_watered";
        case OutcomeMetric::CROP_FERTILIZED: return "crop_fertilized";
        case OutcomeMetric::PASTURE_BUILT: return "pasture_built";
        case OutcomeMetric::COOP_BUILT: return "coop_built";
        case OutcomeMetric::DUG: return "dug";
        case OutcomeMetric::PRODUCT_DEPOSITED: return "product_deposited";
        case OutcomeMetric::END_TILE_STORED: return "end_tile_stored";
        case OutcomeMetric::END_SHED: return "end_shed";
        case OutcomeMetric::END_SEEDS: return "end_seeds";
        case OutcomeMetric::END_CASH: return "end_cash";
        case OutcomeMetric::HIRES: return "hires";
        case OutcomeMetric::PURCHASED: return "purchased";
        case OutcomeMetric::PICKUPS: return "pickups";
        case OutcomeMetric::DROPS: return "drops";
        case OutcomeMetric::UNIT_ACTIONS: return "unit_actions";
        case OutcomeMetric::TRAVEL: return "travel";
        case OutcomeMetric::SPARE_TURNS: return "spare_turns";
        case OutcomeMetric::MIN_SHED_HEADROOM: return "min_shed_headroom";
    }
    throw std::runtime_error("invalid outcome metric");
}

OutcomeMetric parse_metric(const json::value& value, std::string_view path) {
    const std::string name = string(value, path);
    for (int raw = static_cast<int>(OutcomeMetric::PLANTED);
         raw <= static_cast<int>(OutcomeMetric::COOP_BUILT); ++raw) {
        const auto metric = static_cast<OutcomeMetric>(raw);
        if (name == metric_name(metric)) return metric;
    }
    throw std::runtime_error(std::string(path) + " has unknown metric " + name);
}

const char* market_op_name(uint8_t op) {
    switch (op) {
        case kag::M_HIRE: return "hire";
        case kag::M_BUY_LAND: return "buy_land";
        case kag::M_BUY_SEED: return "buy_seed";
        case kag::M_BUY_PRODUCT: return "buy_product";
        case kag::M_BUY_ANIMAL: return "buy_animal";
        case kag::M_SELL: return "sell";
        default: throw std::runtime_error("invalid market op");
    }
}

uint8_t parse_market_op(const json::value& value, std::string_view path) {
    const std::string name = string(value, path);
    for (uint8_t op : {kag::M_HIRE, kag::M_BUY_LAND, kag::M_BUY_SEED,
                       kag::M_BUY_PRODUCT, kag::M_BUY_ANIMAL, kag::M_SELL})
        if (name == market_op_name(op)) return op;
    throw std::runtime_error(std::string(path) +
        " has forbidden or unknown market op " + name);
}

const char* tile_op_name(uint8_t op) {
    switch (op) {
        case kag::OP_PLANT: return "plant";
        case kag::OP_WATER: return "water";
        case kag::OP_HARVEST: return "harvest";
        case kag::OP_FERTILIZE: return "fertilize";
        case kag::OP_DIG: return "dig";
        case kag::OP_BUILD_PASTURE: return "build_pasture";
        case kag::OP_BUILD_COOP: return "build_coop";
        case kag::OP_FEED: return "feed";
        case kag::OP_COLLECT_FERTILIZER: return "collect_fertilizer";
        case kag::OP_CARE: return "care";
        case kag::OP_PLACE: return "place";
        default: throw std::runtime_error("invalid tile-work operation");
    }
}

uint8_t parse_tile_op(const json::value& value, std::string_view path) {
    const std::string name = string(value, path);
    for (uint8_t op : {kag::OP_PLANT, kag::OP_WATER, kag::OP_HARVEST,
                       kag::OP_FERTILIZE, kag::OP_DIG,
                       kag::OP_BUILD_PASTURE, kag::OP_BUILD_COOP, kag::OP_FEED,
                       kag::OP_COLLECT_FERTILIZER, kag::OP_CARE,
                       kag::OP_PLACE})
        if (name == tile_op_name(op)) return op;
    throw std::runtime_error(std::string(path) +
                             " has unknown tile-work operation " + name);
}

ManagedTileState read_tile_state(const json::value& value,
                                 const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"kind", "crop", "animal", "age_days",
        "stored_units", "consecutive_dry_days", "pending_care_bonus",
        "fertilizer_days_remaining", "watered_today", "fed_today",
        "cared_today", "fertilizer_available"}, path);
    ManagedTileState state;
    state.kind = parse_kind(input.at("kind"), path + ".kind");
    state.crop = narrow<int16_t>(integer(input.at("crop"), path + ".crop"), path + ".crop");
    state.animal = narrow<int16_t>(integer(input.at("animal"), path + ".animal"), path + ".animal");
    state.age_days = narrow<int32_t>(integer(input.at("age_days"), path + ".age_days"), path + ".age_days");
    state.stored_units = narrow<int16_t>(integer(input.at("stored_units"), path + ".stored_units"), path + ".stored_units");
    state.consecutive_dry_days = narrow<int32_t>(integer(input.at("consecutive_dry_days"), path + ".consecutive_dry_days"), path + ".consecutive_dry_days");
    state.pending_care_bonus = narrow<int32_t>(integer(input.at("pending_care_bonus"), path + ".pending_care_bonus"), path + ".pending_care_bonus");
    state.fertilizer_days_remaining = narrow<int32_t>(integer(input.at("fertilizer_days_remaining"), path + ".fertilizer_days_remaining"), path + ".fertilizer_days_remaining");
    state.watered_today = boolean(input.at("watered_today"), path + ".watered_today");
    state.fed_today = boolean(input.at("fed_today"), path + ".fed_today");
    state.cared_today = boolean(input.at("cared_today"), path + ".cared_today");
    state.fertilizer_available = boolean(input.at("fertilizer_available"), path + ".fertilizer_available");
    return state;
}

json::object write_tile_state(const ManagedTileState& state) {
    return {{"kind", kind_name(state.kind)}, {"crop", state.crop},
        {"animal", state.animal}, {"age_days", state.age_days},
        {"stored_units", state.stored_units},
        {"consecutive_dry_days", state.consecutive_dry_days},
        {"pending_care_bonus", state.pending_care_bonus},
        {"fertilizer_days_remaining", state.fertilizer_days_remaining},
        {"watered_today", state.watered_today},
        {"fed_today", state.fed_today}, {"cared_today", state.cared_today},
        {"fertilizer_available", state.fertilizer_available}};
}

ManagedTile read_tile(const json::value& value, const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"x", "y", "state"}, path);
    return {
        narrow<int8_t>(integer(input.at("x"), path + ".x"), path + ".x"),
        narrow<int8_t>(integer(input.at("y"), path + ".y"), path + ".y"),
        read_tile_state(input.at("state"), path + ".state"),
    };
}

json::object write_tile(const ManagedTile& tile) {
    return {{"x", tile.x}, {"y", tile.y},
            {"state", write_tile_state(tile.state)}};
}

PhysicalState read_start(const json::value& value) {
    const auto& input = object(value, "start");
    exact_keys(input, {"managed_tiles", "shed", "seeds"}, "start");
    PhysicalState result;
    const auto& tiles = array(input.at("managed_tiles"), "start.managed_tiles");
    result.managed_tiles.reserve(tiles.size());
    for (size_t index = 0; index < tiles.size(); ++index)
        result.managed_tiles.push_back(read_tile(
            tiles[index], "start.managed_tiles[" + std::to_string(index) + "]"));
    result.shed = read_inventory<kag::N_ITEMS>(input.at("shed"), "start.shed");
    result.seeds = read_inventory<kag::N_CROPS>(input.at("seeds"), "start.seeds");
    result.shed_capacity = std::numeric_limits<int16_t>::max();
    result.cash = 0;
    return result;
}

json::object write_start(const PhysicalState& start) {
    json::array tiles;
    tiles.reserve(start.managed_tiles.size());
    for (const auto& tile : start.managed_tiles)
        tiles.push_back(write_tile(tile));
    return {{"managed_tiles", std::move(tiles)},
            {"shed", write_fixed_array(start.shed)},
            {"seeds", write_fixed_array(start.seeds)}};
}

OutcomeKey read_key(const json::value& value, const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"metric", "subject", "tile", "through_hour"}, path);
    return {
        parse_metric(input.at("metric"), path + ".metric"),
        narrow<int16_t>(integer(input.at("subject"), path + ".subject"), path + ".subject"),
        narrow<int16_t>(integer(input.at("tile"), path + ".tile"), path + ".tile"),
        narrow<int8_t>(integer(input.at("through_hour"), path + ".through_hour"), path + ".through_hour"),
    };
}

json::object write_key(const OutcomeKey& key) {
    return {{"metric", metric_name(key.metric)}, {"subject", key.subject},
            {"tile", key.tile}, {"through_hour", key.through_hour}};
}

OutcomeBound read_bound(const json::value& value, const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"key", "lower", "upper"}, path);
    return {read_key(input.at("key"), path + ".key"),
            integer(input.at("lower"), path + ".lower"),
            integer(input.at("upper"), path + ".upper")};
}

json::object write_bound(const OutcomeBound& bound) {
    return {{"key", write_key(bound.key)},
            {"lower", bound.lower}, {"upper", bound.upper}};
}

EndTileRequirement read_end_tile(const json::value& value,
                                 const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"tile", "state"}, path);
    EndTileRequirement result;
    result.tile = narrow<int16_t>(
        integer(input.at("tile"), path + ".tile"), path + ".tile");
    result.exact_state = read_tile_state(input.at("state"), path + ".state");
    result.kind = result.exact_state->kind;
    if (result.kind == ManagedTileKind::CROP)
        result.item = result.exact_state->crop;
    else if (animal_structure(result.kind))
        result.item = result.exact_state->animal;
    return result;
}

json::object write_end_tile(const EndTileRequirement& value) {
    return {{"tile", value.tile},
            {"state", write_tile_state(*value.exact_state)}};
}

SaleTarget read_sale_target(const json::value& value,
                            const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"market_event"}, path);
    SaleTarget result;
    result.market_event = narrow<uint16_t>(
        integer(input.at("market_event"), path + ".market_event"),
        path + ".market_event");
    return result;
}

json::object write_sale_target(const SaleTarget& value) {
    return {{"market_event", value.market_event}};
}

MarketEvent read_market_event(const json::value& value,
                              const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"hour", "order_index", "op", "item", "quantity"},
               path);
    MarketEvent result;
    result.hour = narrow<int8_t>(integer(input.at("hour"), path + ".hour"),
                                 path + ".hour");
    result.order_index = narrow<int8_t>(
        integer(input.at("order_index"), path + ".order_index"),
        path + ".order_index");
    result.market_op = parse_market_op(input.at("op"), path + ".op");
    result.item = narrow<int16_t>(integer(input.at("item"), path + ".item"),
                                  path + ".item");
    result.quantity = narrow<int32_t>(
        integer(input.at("quantity"), path + ".quantity"),
        path + ".quantity");
    result.cash_delta = 0;
    return result;
}

json::object write_market_event(const MarketEvent& value) {
    return {{"hour", value.hour}, {"order_index", value.order_index},
            {"op", market_op_name(value.market_op)}, {"item", value.item},
            {"quantity", value.quantity}};
}

TileWorkAction read_tile_work_action(const json::value& value,
                                     const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"op", "arg", "quantity", "output_item",
                       "output_quantity"}, path);
    TileWorkAction result;
    result.op = parse_tile_op(input.at("op"), path + ".op");
    result.arg = narrow<int16_t>(integer(input.at("arg"), path + ".arg"),
                                 path + ".arg");
    result.quantity = narrow<int32_t>(
        integer(input.at("quantity"), path + ".quantity"),
        path + ".quantity");
    result.output_item = narrow<int16_t>(
        integer(input.at("output_item"), path + ".output_item"),
        path + ".output_item");
    result.output_quantity = narrow<int32_t>(
        integer(input.at("output_quantity"), path + ".output_quantity"),
        path + ".output_quantity");
    return result;
}

json::object write_tile_work_action(const TileWorkAction& value) {
    return {{"op", tile_op_name(value.op)}, {"arg", value.arg},
            {"quantity", value.quantity},
            {"output_item", value.output_item},
            {"output_quantity", value.output_quantity}};
}

TileWork read_tile_work(const json::value& value, const std::string& path) {
    const auto& input = object(value, path);
    exact_keys(input, {"tile", "actions"}, path);
    TileWork result;
    result.tile = narrow<int16_t>(integer(input.at("tile"), path + ".tile"),
                                  path + ".tile");
    const auto& actions = array(input.at("actions"), path + ".actions");
    result.actions.reserve(actions.size());
    for (size_t index = 0; index < actions.size(); ++index)
        result.actions.push_back(read_tile_work_action(
            actions[index], path + ".actions[" + std::to_string(index) + "]"));
    return result;
}

json::object write_tile_work(const TileWork& value) {
    json::array actions;
    for (const auto& action : value.actions)
        actions.push_back(write_tile_work_action(action));
    return {{"tile", value.tile}, {"actions", std::move(actions)}};
}

template <class T, class Reader>
std::vector<T> read_vector(const json::value& value, const std::string& path,
                           Reader reader) {
    const auto& input = array(value, path);
    std::vector<T> result;
    result.reserve(input.size());
    for (size_t index = 0; index < input.size(); ++index)
        result.push_back(reader(input[index], path + "[" +
                                std::to_string(index) + "]"));
    return result;
}

}  // namespace

DayProblem parse_problem_json(std::string_view text) {
    const json::value parsed = json::parse(text);
    const auto& input = object(parsed, "root");
    exact_keys(input, {"format_version", "worker_count", "start",
        "end_tiles", "tile_work", "buy_schedule", "shed_availability",
        "end_shed", "end_seeds"}, "root");

    DayProblem result;
    result.format_version = narrow<uint32_t>(
        integer(input.at("format_version"), "format_version"),
        "format_version");
    result.start = read_start(input.at("start"));
    result.worker_count = narrow<uint16_t>(
        integer(input.at("worker_count"), "worker_count"), "worker_count");
    result.required_end_tiles = read_vector<EndTileRequirement>(
        input.at("end_tiles"), "end_tiles", read_end_tile);
    result.tile_work = read_vector<TileWork>(
        input.at("tile_work"), "tile_work", read_tile_work);
    result.market_plan = read_vector<MarketEvent>(
        input.at("buy_schedule"), "buy_schedule", read_market_event);
    const auto& availability = array(input.at("shed_availability"),
                                     "shed_availability");
    if (availability.size() != HOURS)
        throw std::runtime_error("shed_availability has the wrong length");
    for (int hour = 0; hour < HOURS; ++hour) {
        result.shed_availability[hour] =
            read_inventory<kag::N_ITEMS>(
                availability[hour], "shed_availability[" +
                std::to_string(hour) + "]");
    }
    result.end_shed = read_inventory<kag::N_ITEMS>(
        input.at("end_shed"), "end_shed");
    result.end_seeds = read_inventory<kag::N_CROPS>(
        input.at("end_seeds"), "end_seeds");

    // Keep exact terminal inventory bounds for legacy solver components while
    // the public v3 contract uses the explicit arrays above.
    for (int item = 0; item < kag::N_ITEMS; ++item)
        result.required_outcomes.push_back({
            {OutcomeMetric::END_SHED, static_cast<int16_t>(item), NO_TILE, -1},
            result.end_shed[item], result.end_shed[item]});
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        result.required_outcomes.push_back({
            {OutcomeMetric::END_SEEDS, static_cast<int16_t>(crop), NO_TILE, -1},
            result.end_seeds[crop], result.end_seeds[crop]});

    for (const MarketEvent& event : result.market_plan) {
        if (event.market_op != kag::M_BUY_SEED &&
            event.market_op != kag::M_BUY_PRODUCT &&
            event.market_op != kag::M_BUY_ANIMAL)
            continue;
        AllowedAcquisition acquisition;
        acquisition.market_op = event.market_op;
        acquisition.item = event.item;
        acquisition.lower = event.quantity;
        acquisition.upper = event.quantity;
        acquisition.first_hour = event.hour;
        acquisition.last_hour = event.hour;
        result.allowed_acquisitions.push_back(std::move(acquisition));
    }

    const auto issues = validate_problem(result);
    if (!issues.empty())
        throw std::runtime_error(issues.front().path + ": " +
                                 issues.front().message);
    return result;
}

std::string serialize_problem_json(const DayProblem& problem) {
    const auto issues = validate_problem(problem);
    if (!issues.empty())
        throw std::runtime_error(issues.front().path + ": " +
                                 issues.front().message);

    json::array end_tiles;
    for (const auto& value : problem.required_end_tiles)
        end_tiles.push_back(write_end_tile(value));
    json::array tile_work;
    for (const auto& value : problem.tile_work)
        tile_work.push_back(write_tile_work(value));
    json::array buy_schedule;
    for (const auto& value : problem.market_plan)
        buy_schedule.push_back(write_market_event(value));
    json::array availability;
    for (const auto& hour : problem.shed_availability)
        availability.push_back(write_fixed_array(hour));

    json::object root{
        {"format_version", problem.format_version},
        {"worker_count", problem.worker_count},
        {"start", write_start(problem.start)},
        {"end_tiles", std::move(end_tiles)},
        {"tile_work", std::move(tile_work)},
        {"buy_schedule", std::move(buy_schedule)},
        {"shed_availability", std::move(availability)},
        {"end_shed", write_fixed_array(problem.end_shed)},
        {"end_seeds", write_fixed_array(problem.end_seeds)},
    };
    return json::serialize(root) + "\n";
}

DayProblem load_problem_json(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open " + path.string());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse_problem_json(buffer.str());
}

void save_problem_json(const DayProblem& problem,
                       const std::filesystem::path& path) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot open " + path.string());
    output << serialize_problem_json(problem);
    if (!output) throw std::runtime_error("cannot write " + path.string());
}

}  // namespace day_solver
