#pragma once
#include "trading.hpp"
#include <array>

namespace kag::day_compiler {
constexpr int INVENTORY_EVENTS=32;
struct InventoryEvent {
    int rival=0, rival_after=0, demand_after=0;
    int net_receipt_next=0;
    int minimum_left=0, maximum_left=100;
    bool trade_allowed=true;
    // Cash available from everything except the controller's cumulative trades.
    double cash_available=0;
};
struct InventoryProblem {
    int product=WHEAT, count=0, capacity=100;
    InventoryEvent events[INVENTORY_EVENTS]{};
    double rival_weight=0;
    bool terminal=false;
    int preferred_first_quantity=0; // Exact-value tie break only; positive sells.
    struct Residual {
        double probability=0;
        int inventory_change=0, usable_receipts=0, requirement=0;
    };
    Residual residual[4]{};
    uint64_t transition_limit=500000;
};
struct InventoryChoice {
    int quantity=0; // Positive sells, negative buys.
    double value=-1e100;
    uint64_t transitions=0;
    bool complete=true, feasible=false, reduced=false;
};
struct InventoryCurve {
    std::array<double,101> value{}; // Value for each post-trade stock quantity.
    uint64_t transitions=0;
    bool complete=true;
    InventoryCurve() { value.fill(-1e100); }
};
// Exact conditional inventory control. The O(HQ) reduction is used only when
// every reachable quote is above the floor and there is no rival trade. General
// states preserve both cash and margin; budget exhaustion is an explicit failure.
class InventoryController {
public:
    InventoryChoice solve(const InventoryProblem& problem,int stock,int inventory,bool allow_reduction=true);
    InventoryCurve curve(const InventoryProblem& problem,int stock,int inventory);
private:
    struct Label {
        int stock=0, inventory=0, first=0, next=-1;
        double cash=0, value=0;
    };
    struct CashLabel { double required=0, value=-1e100; };
    static constexpr int LABELS=4096, HASH=8192;
    static constexpr int CASH_LABELS=32;
    std::array<Label,LABELS> current_{}, next_{};
    std::array<int,HASH> heads_{};
    CashLabel cash_current_[101][CASH_LABELS]{}, cash_next_[101][CASH_LABELS]{};
    InventoryChoice reduced(const InventoryProblem& problem,int stock,int inventory);
    InventoryChoice general(const InventoryProblem& problem,int stock,int inventory);
    InventoryCurve conserved_curve(const InventoryProblem& problem,int stock,int inventory);
};
}
