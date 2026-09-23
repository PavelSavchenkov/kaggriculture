#pragma once
#include "trading.hpp"
#include "worker/source/policy.hpp"

namespace kag::day_compiler {
struct ResourceSchedule {
    static constexpr int MAX_SHED_TRANSFERS=MAX_UNITS*(N_ITEMS+48);
    struct ShedTransfer { uint8_t product=0; int quantity=0; }; // Signed, worker/item order.
    ShedTransfer shed_transfers[MAX_SHED_TRANSFERS]{};
    int shed_transfer_begin[25]{}, shed_transfer_count=0;
    Action workers[24]{};
    int hours=24, start_hour=0;
    int receipts[24][N_ITEMS]{}, pickups[24][N_ITEMS]{}, purchases[24][N_ITEMS]{};
    int fixed_sales[24][N_PRODUCTS]{};
    int night_returns[N_ITEMS]{}, night_input_stock[N_ITEMS]{};
    struct NightDeposit { uint8_t product=0; int quantity=0; };
    NightDeposit night_deposits[MAX_UNITS*N_ITEMS]{};
    int night_deposit_count=0;
    Tile next_dawn_tiles[100]{}; // Own biological projection from today's program only.
    int reserve_after_market[24][N_ITEMS]{};
    int input_next_need[24][2]{}, input_buy_need[24][2]{}, input_remaining_need[24][2]{}; // Wheat, fertilizer.
    int capacity_after_market[24]{};
    int required_workers[24]{};
    bool preserve_input_orders=false;
    bool flexible_inputs=false, planned_input_purchases=false;
    ResourceSchedule();
};
// Capture resource events from a verified worker schedule, beginning at the
// observation's actual hour. Indices remain absolute day hours. Purchases need
// separate funding and full-engine verification.
bool describe_resources(const Observation& dawn,const Action* workers,ResourceSchedule& resources,
                        const Configuration& config={},bool plan_input_purchases=false);
struct NightSettlement {
    int receipts[N_ITEMS]{};
    int capacity_before=100, locked_stock=0, unavoidable_discard=0;
    bool valid=true;
};
// Maximal feasible automatic deposit after selling available finished stock.
// Preserve exact worker/item deposit order when unavoidable overflow occurs.
NightSettlement settle_night(const ResourceSchedule& resources,const Configuration& config={});
enum class ExecutionError { None, Workforce, WorkerEffect, Funding, Capacity, MarketSlots, MarketPlan };
ExecutionError liquidation_orders(const Observation& observation,const Farm& after_workers,
                                  const ResourceSchedule& resources,Action& action,const Configuration& config={});
}
