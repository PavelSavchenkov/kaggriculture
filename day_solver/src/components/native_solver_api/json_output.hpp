#pragma once
// Adapter-only JSON/file helpers, copied from the closed CLI.
json::array schedule_json(const std::array<kag::Action, hours>& actions) {
    static constexpr const char* item_names[] = {"WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP"};
    static const std::map<int, const char*> operations{{kag::OP_PASS,"PASS"},{kag::OP_NORTH,"NORTH"},{kag::OP_SOUTH,"SOUTH"},
        {kag::OP_EAST,"EAST"},{kag::OP_WEST,"WEST"},{kag::OP_PICKUP,"PICKUP"},{kag::OP_DROP,"DROP"},
        {kag::OP_PLACE,"PLACE"},{kag::OP_PLANT,"PLANT"},{kag::OP_WATER,"WATER"},{kag::OP_HARVEST,"HARVEST"},
        {kag::OP_FERTILIZE,"FERTILIZE"},{kag::OP_DIG,"DIG"},{kag::OP_BUILD_COOP,"BUILD_COOP"},
        {kag::OP_BUILD_PASTURE,"BUILD_PASTURE"},{kag::OP_FEED,"FEED"},{kag::OP_COLLECT_FERTILIZER,"COLLECT_FERTILIZER"},{kag::OP_CARE,"CARE"}};
    static const std::map<int, const char*> market{{kag::M_NONE,"NONE"},{kag::M_HIRE,"HIRE"},{kag::M_BUY_LAND,"BUY_LAND"},
        {kag::M_BUY_SEED,"BUY_SEED"},{kag::M_BUY_PRODUCT,"BUY_PRODUCT"},{kag::M_BUY_ANIMAL,"BUY_ANIMAL"}};
    json::array result;
    for (const auto& turn : actions) {
        json::array units, orders;
        for (int worker = 0; worker < turn.n_units; ++worker) {
            const auto& action = turn.units[worker];
            json::array command{operations.at(action.op)};
            if (action.op == kag::OP_PLANT || action.op == kag::OP_PICKUP || action.op == kag::OP_PLACE) command.push_back(item_names[action.arg]);
            if (action.op == kag::OP_PICKUP || (action.op == kag::OP_PLACE && action.arg < kag::GOOSE)) command.push_back(action.n);
            units.push_back(std::move(command));
        }
        for (int index = 0; index < turn.n_orders; ++index) {
            const auto& order = turn.orders[index];
            json::array command{market.at(order.op)};
            if (order.op != kag::M_NONE && order.op != kag::M_HIRE && order.op != kag::M_BUY_LAND) {
                command.push_back(item_names[order.item]); command.push_back(order.n);
            }
            orders.push_back(std::move(command));
        }
        result.push_back({{"units", std::move(units)}, {"orders", std::move(orders)}});
    }
    return result;
}

void write_json(const fs::path& path, const json::value& value) {
    std::ofstream output(path);
    require(bool(output), "cannot write " + path.string());
    output << json::serialize(value) << '\n';
    require(bool(output), "write failed " + path.string());
}

