#pragma once
#include "trading.hpp"
#include <array>

namespace kag::day_compiler {
struct SaleCurve {
    int product=0, stock=0;
    int maximum_buy=0;
    bool existing_sale_order=false;
    std::array<double,101> value;
    std::array<double,101> buy_value;
    SaleCurve() { value.fill(-1e100); buy_value.fill(-1e100); }
};
struct SaleAllocation {
    int quantity[N_PRODUCTS]{}; // Positive sells, negative buys.
    double value=-1e100;
    bool feasible=false;
};
// Exact current-market allocation conditional on the supplied continuation
// curves. Future shared constraints still belong to those conditional models.
SaleAllocation allocate_sales(const SaleCurve* curves,int count,int occupied_other,int capacity,int order_slots);
}
