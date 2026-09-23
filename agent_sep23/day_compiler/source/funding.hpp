#pragma once
#include "trading.hpp"
#include "workload.hpp"

namespace kag::day_compiler {
struct FundingScenario { int rival_sales[24][N_PRODUCTS]{}; };
struct FundingProposal {
    worker::DayInput input{};
    worker::SolveOptions constraints{};
    int dawn_reserve[N_PRODUCTS]{};
    int input_returns[N_PRODUCTS]{}, financing_returns[N_PRODUCTS]{};
    int funding_hour=0;
    double mandatory_bill=0, dawn_liquid_cash=0;
    FundingProposal();
};
// Produce a purchase/return problem for the existing movement solver. This is
// a proposal, never an accepted compiler result until full engine verification.
// Suffix calls pass their actual hour as funding_hour (or a later return hour).
bool propose_funding(const Observation& dawn,const detail::BoundIntent& intent,const Workload& workload,
                     FundingProposal& out,int funding_hour=0,const Configuration& config={});
// Greedy cash-flow proposal: finance concrete purchases with nearby output
// batches, and release each hire only after its wage is covered.
bool propose_cashflow(const Observation& dawn,const detail::BoundIntent& intent,const Workload& workload,
                      FundingProposal& out,int hires,int receipt_slack,const Configuration& config={},
                      const FundingScenario& scenario={});
}
