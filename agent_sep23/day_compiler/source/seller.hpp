#pragma once
#include "trading.hpp"
#include <array>

namespace kag::day_compiler {
constexpr int SALE_EVENTS=32;
struct SaleEvent {
    int rival=0, rival_after=0, demand_after=0;
    int receipt_next=0, use_next=0;
    int minimum_sale=0, minimum_left=0, maximum_left=100;
    int elapsed=0;
};
struct SaleProblem {
    int product=CARROT, count=0;
    SaleEvent events[SALE_EVENTS]{};
    double rival_weight=1;
    double holding_per_hour=0;
    int terminal_stock=0; // -1 permits residual stock, valued by the scenarios below.
    struct Residual { double probability=0; int inventory_change=0, requirement=0; };
    Residual residual[4]{};
    uint64_t transition_limit=500000;
};
struct SaleChoice {
    int quantity=0;
    double value=-1e100, wait_value=-1e100;
    std::array<double,101> sale_values{};
    uint64_t states=0, transitions=0;
    bool complete=true, feasible=false;
};
// Exact integer sale DP conditional on a fixed rival trajectory and resource
// constraints. A joint order scheduler must enforce shared cash/capacity/slots.
class Seller {
public:
    SaleChoice solve(const SaleProblem& problem,int stock,int inventory);
private:
    struct Entry { uint64_t key=0; double value=0; uint32_t generation=0; };
    std::array<Entry,32768> cache_{};
    uint32_t generation_=0;
    const SaleProblem* problem_=nullptr;
    SaleChoice result_{};
    double value(int event,int stock,int inventory);
};
}
