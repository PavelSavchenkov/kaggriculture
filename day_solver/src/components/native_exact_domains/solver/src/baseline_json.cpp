#include "baseline_json.hpp"

#include <boost/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace day_solver {
namespace {

namespace json = boost::json;

int item_id(std::string_view name) {
    static constexpr std::string_view names[kag::N_ITEMS] = {
        "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG",
        "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP"};
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (name == names[item]) return item;
    throw std::runtime_error("unknown item " + std::string(name));
}

int64_t integer(const json::value& value) {
    if (value.is_int64()) return value.as_int64();
    if (value.is_uint64()) return static_cast<int64_t>(value.as_uint64());
    throw std::runtime_error("command quantity must be an integer");
}

std::string_view string(const json::value& value) {
    if (!value.is_string()) throw std::runtime_error("command token must be a string");
    return value.as_string();
}

uint8_t unit_op(std::string_view name) {
    static constexpr std::pair<std::string_view, uint8_t> values[] = {
        {"PASS", kag::OP_PASS}, {"NORTH", kag::OP_NORTH},
        {"SOUTH", kag::OP_SOUTH}, {"EAST", kag::OP_EAST},
        {"WEST", kag::OP_WEST}, {"PICKUP", kag::OP_PICKUP},
        {"DROP", kag::OP_DROP}, {"PLACE", kag::OP_PLACE},
        {"PLANT", kag::OP_PLANT}, {"WATER", kag::OP_WATER},
        {"HARVEST", kag::OP_HARVEST}, {"FERTILIZE", kag::OP_FERTILIZE},
        {"DIG", kag::OP_DIG}, {"BUILD_COOP", kag::OP_BUILD_COOP},
        {"BUILD_PASTURE", kag::OP_BUILD_PASTURE}, {"FEED", kag::OP_FEED},
        {"COLLECT_FERTILIZER", kag::OP_COLLECT_FERTILIZER},
        {"CARE", kag::OP_CARE},
    };
    for (const auto& [candidate, op] : values)
        if (name == candidate) return op;
    throw std::runtime_error("unknown unit operation " + std::string(name));
}

uint8_t market_op(std::string_view name) {
    static constexpr std::pair<std::string_view, uint8_t> values[] = {
        {"NONE", kag::M_NONE}, {"HIRE", kag::M_HIRE},
        {"BUY_LAND", kag::M_BUY_LAND},
        {"BUY_SEED", kag::M_BUY_SEED},
        {"BUY_PRODUCT", kag::M_BUY_PRODUCT},
        {"BUY_ANIMAL", kag::M_BUY_ANIMAL},
        {"SELL", kag::M_SELL},
    };
    for (const auto& [candidate, op] : values)
        if (name == candidate) return op;
    throw std::runtime_error("unknown market operation " + std::string(name));
}

kag::UnitAction parse_unit(const json::value& value) {
    if (!value.is_array() || value.as_array().empty())
        throw std::runtime_error("unit command must be a nonempty array");
    const json::array& command = value.as_array();
    kag::UnitAction result;
    result.op = unit_op(string(command[0]));
    if (result.op == kag::OP_PICKUP || result.op == kag::OP_PLACE ||
        result.op == kag::OP_PLANT) {
        if (command.size() < 2)
            throw std::runtime_error("unit command is missing its item");
        result.arg = static_cast<uint8_t>(item_id(string(command[1])));
    }
    if ((result.op == kag::OP_PICKUP || result.op == kag::OP_PLACE) &&
        command.size() >= 3)
        result.n = static_cast<int32_t>(integer(command[2]));
    return result;
}

kag::Order parse_order(const json::value& value) {
    if (!value.is_array() || value.as_array().empty())
        throw std::runtime_error("market command must be a nonempty array");
    const json::array& command = value.as_array();
    kag::Order result;
    result.op = market_op(string(command[0]));
    if (result.op == kag::M_NONE || result.op == kag::M_HIRE ||
        result.op == kag::M_BUY_LAND) return result;
    if (command.size() != 3)
        throw std::runtime_error("purchase command needs item and quantity");
    result.item = static_cast<uint8_t>(item_id(string(command[1])));
    result.n = static_cast<int32_t>(integer(command[2]));
    return result;
}

}  // namespace

std::array<kag::Action, HOURS> load_baseline_json(
    const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open " + path.string());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const json::value root = json::parse(buffer.str());
    if (!root.is_array() || root.as_array().size() != HOURS)
        throw std::runtime_error("baseline must contain 24 hour actions");

    std::array<kag::Action, HOURS> result{};
    for (int hour = 0; hour < HOURS; ++hour) {
        const json::value& entry = root.as_array()[hour];
        if (!entry.is_object())
            throw std::runtime_error("baseline hour must be an object");
        const json::object& object = entry.as_object();
        const json::array& units = object.at("units").as_array();
        const json::array& orders = object.at("orders").as_array();
        if (units.empty() || units.size() > kag::MAX_UNITS || orders.size() > 10)
            throw std::runtime_error("baseline action has invalid width");
        kag::Action& action = result[hour];
        action.n_units = static_cast<int>(units.size());
        for (size_t unit = 0; unit < units.size(); ++unit)
            action.units[unit] = parse_unit(units[unit]);
        action.n_orders = static_cast<int>(orders.size());
        for (size_t order = 0; order < orders.size(); ++order)
            action.orders[order] = parse_order(orders[order]);
        action.finalize();
    }
    return result;
}

}  // namespace day_solver
