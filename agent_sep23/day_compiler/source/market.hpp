#pragma once
#include "history.hpp"
#include "schedule.hpp"
#include "seller.hpp"
#include "inventory.hpp"

namespace kag::day_compiler {
enum class MarketMode { Liquidate, SellerZeroFlow, SellerRecentFlow, SellerModelFlow, SellerTerminalFlow,
                        SellerTerminalWheat, SellerTerminalWheatJoint, SellerTerminalJoint,
                        SellerTerminalWheatSpace, SellerTerminalSpace, SellerTerminalInventory,
                        SellerTerminalInventoryFlow, SellerTerminalInventoryFlowCash, SellerTerminalInventoryRecent,
                        SellerRollingH6, SellerRollingH3, SellerRollingStockH6, SellerRollingStockH3 };
constexpr bool controls_wheat(MarketMode mode) {
    return mode>=MarketMode::SellerTerminalInventory && mode<=MarketMode::SellerTerminalInventoryRecent;
}
constexpr bool uses_rolling_sales(MarketMode mode) {
    return mode>=MarketMode::SellerRollingH6 && mode<=MarketMode::SellerRollingStockH3;
}
constexpr bool uses_planned_sale_receipts(MarketMode mode) {
    return mode==MarketMode::SellerRollingH6 || mode==MarketMode::SellerRollingH3;
}
constexpr bool uses_model(MarketMode mode) { return mode>=MarketMode::SellerModelFlow; }
constexpr bool uses_terminal_balance(MarketMode mode) { return mode>=MarketMode::SellerTerminalFlow; }
constexpr bool uses_wheat_dp(MarketMode mode) {
    return mode==MarketMode::SellerTerminalWheat || mode==MarketMode::SellerTerminalWheatJoint ||
           mode==MarketMode::SellerTerminalWheatSpace || controls_wheat(mode);
}
constexpr bool uses_joint_sales(MarketMode mode) { return mode>=MarketMode::SellerTerminalWheatJoint; }
class MarketPlanner {
public:
    ExecutionError orders(const Observation& observation,const History& history,const Farm& after_workers,
                          const ResourceSchedule& resources,MarketMode mode,Action& action,const Configuration& config={});
    InventoryCurve inventory_diagnostics{};
    struct Failure {
        int stage=0,product=-1,capacity=0,total=0,slots=0,count=0;
        int products[N_PRODUCTS]{},minimum_sale[N_PRODUCTS]{},stock[N_PRODUCTS]{};
        bool reusable[N_PRODUCTS]{};
        Action orders{};
    } failure;
private:
    Seller seller_;
    InventoryController inventory_;
};
}
