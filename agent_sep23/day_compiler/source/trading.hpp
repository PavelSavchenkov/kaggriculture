#pragma once
#include "state.hpp"

namespace kag::day_compiler {
struct Transaction {
    int inventory = 0;
    double own_cash = 0, rival_cash = 0;
};
// Signed accepted quantities: positive sells, negative buys. Affordability and
// shed space are separate constraints; both players quote before either commits.
Transaction transact(int product, int inventory, int own_quantity, int rival_quantity);
// A quote envelope for one wheat order and any within-hour rival buy/sell
// sequence with bounded shed stock. Positive sells, negative buys.
int wheat_inventory_bound(int inventory,int quantity,int rival_capacity);
// Finished goods cannot be bought in this market phase. Selling their existing
// stock first releases cash/space while preserving all other order dependencies.
bool finished_sales_first(const Observation& observation,const Farm& after_workers,Action& action,
                          const Configuration& config={});
// Keep leading product sales first, then sell existing fertilizer before other
// orders. A fertilizer sale following its purchase retains that dependency.
bool fertilizer_sales_after_products(const Observation& observation,const Farm& after_workers,Action& action,
                                    const Configuration& config={});

class OrderLedger {
public:
    OrderLedger(const Observation& observation, const Farm& after_workers, const Configuration& config = {});
    bool append(Order order);
    bool extend_finished_sale(int product,int quantity);
    double cash = 0;
    int stock[N_ITEMS]{}, seeds[N_CROPS]{}, inventory[N_PRODUCTS]{};
    int total = 0, hires = 0, quadrants = 1;
    Action orders{};
private:
    Configuration config_{};
};
}
