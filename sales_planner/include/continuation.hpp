#pragma once
#include "baseline.hpp"
#include "compact.hpp"
#include "purchase_repair.hpp"

namespace sales_planner {
struct ShopArrival { int turn = 0; uint8_t type = 0; };
// A forecast is a supplied possible world, never the hidden evaluation world.
// Arrays use absolute turn indices. The rival starts before current workers;
// the own observation is after current workers. Empty rival arrays mean PASS.
struct MarketForecast {
    Account rival;
    Resources rival_resources;
    std::span<const CalendarTurn> rival_calendar;
    std::span<const Orders> rival_orders;
    std::array<ShopArrival, 8> arrivals{};
    int n_arrivals = 0;
};

struct ContinuationResult {
    std::array<Account, 2> accounts;
    std::array<Resources, 2> resources;
    // After the supplied period, before end_turn's shop arrivals and work.
    // These fields let a caller price or resume the remaining period.
    std::array<int, kag::N_PRODUCTS> inventory{};
    std::array<uint8_t, kag::N_SHOPS> shops{};
    int n_shops = 0, end_turn = 0;
    std::array<int, 2> commitment_errors{};
    int evaluated_turns = 0;
    std::array<PurchaseRepairStats,2> repairs{};
    double margin() const { return accounts[0].cash - accounts[1].cash; }
    bool feasible() const {
        for (int p = 0; p < 2; ++p) {
            if (commitment_errors[p]) return false;
            for (int n : resources[p].missing) if (n) return false;
        }
        return true;
    }
};

// Optional offline diagnostics. A mismatch may be an extra successful hire,
// not only a funding shortage; preserve that distinction in reports.
struct ContinuationTrace {
    std::array<int,2> first_missing_turn{-1,-1}, first_missing_item{-1,-1}, first_missing_phase{-1,-1};
    std::array<int,2> first_commitment_turn{-1,-1}, wanted_hires{}, actual_hires{}, wanted_land{}, actual_land{};
    std::array<int,2> first_short_order_turn{-1,-1}, short_order_op{}, short_order_item{}, requested{}, accepted{};
    std::array<double,2> cash_at_missing{}, cash_at_commitment{}, cash_at_short_order{};
    void resources_at(const MarketState& state,const ContinuationResult& result,int phase) {
        for(int p=0;p<2;++p)if(first_missing_turn[p]<0)
            for(int i=0;i<kag::N_ITEMS;++i)if(result.resources[p].missing[i]) {
                first_missing_turn[p]=state.turn;first_missing_item[p]=i;first_missing_phase[p]=phase;
                cash_at_missing[p]=state.accounts[p].cash;break;
            }
    }
    void orders_at(const MarketState& state,int p,const Orders& required,const Orders& actual,const std::array<int,16>& filled) {
        int want[3]{},got[3]{};
        for(int k=0;k<required.count;++k) {
            const int op=required.values[k].op;
            if(op==kag::M_HIRE || op==kag::M_BUY_LAND)++want[op];
        }
        for(int k=0;k<actual.count;++k) {
            const auto o=actual.values[k];
            if(o.op==kag::M_HIRE || o.op==kag::M_BUY_LAND)got[o.op]+=filled[k];
            if(first_short_order_turn[p]<0 && o.op>=kag::M_BUY_SEED && o.op<=kag::M_BUY_ANIMAL && filled[k]<o.n) {
                first_short_order_turn[p]=state.turn;short_order_op[p]=o.op;short_order_item[p]=o.item;
                requested[p]=o.n;accepted[p]=filled[k];cash_at_short_order[p]=state.accounts[p].cash;
            }
        }
        if(first_commitment_turn[p]<0 && (want[kag::M_HIRE]!=got[kag::M_HIRE] || want[kag::M_BUY_LAND]!=got[kag::M_BUY_LAND])) {
            first_commitment_turn[p]=state.turn;wanted_hires[p]=want[kag::M_HIRE];actual_hires[p]=got[kag::M_HIRE];
            wanted_land[p]=want[kag::M_BUY_LAND];actual_land[p]=got[kag::M_BUY_LAND];cash_at_commitment[p]=state.accounts[p].cash;
        }
    }
};

inline int commitment_errors(const Orders& required, const Orders& actual,
                             const std::array<int, 16>& accepted) {
    int want[3]{}, got[3]{};
    for (int k = 0; k < required.count; ++k) {
        const auto op = required.values[k].op;
        if (op == kag::M_HIRE || op == kag::M_BUY_LAND) ++want[op];
    }
    for (int k = 0; k < actual.count; ++k) {
        const auto op = actual.values[k].op;
        if (op == kag::M_HIRE || op == kag::M_BUY_LAND) got[op] += accepted[k];
    }
    return std::abs(want[kag::M_HIRE] - got[kag::M_HIRE]) +
           std::abs(want[kag::M_BUY_LAND] - got[kag::M_BUY_LAND]);
}

// Replay through end of the supplied period. The caller owns any continuation
// value after that period. No routes, tiles, hidden seed or case identity here.
inline ContinuationResult evaluate_continuation(
        const PlannerObservation& start, std::span<const CalendarTurn> calendar,
        std::span<const Orders> warm_orders, const MarketForecast& forecast,
        const MarketRules& rules, const Orders* first_orders = nullptr, bool compact = true,
        ContinuationTrace* trace = nullptr,int repair_sides=0) {
    if (calendar.size() != warm_orders.size() || start.turn >= int(calendar.size())) std::abort();
    if (!forecast.rival_calendar.empty() && forecast.rival_calendar.size() < calendar.size()) std::abort();
    if (!forecast.rival_orders.empty() && forecast.rival_orders.size() < calendar.size()) std::abort();
    MarketState state;
    state.turn = start.turn; state.accounts = {start.own, forecast.rival};
    state.inventory = start.inventory; state.shops = start.shops; state.n_shops = start.n_shops;
    ContinuationResult result;
    result.resources = {start.resources, forecast.rival_resources};
    int arrival = 0;
    for (int t = start.turn; t < int(calendar.size()); ++t) {
        while (arrival < forecast.n_arrivals && forecast.arrivals[arrival].turn <= t) {
            const auto event = forecast.arrivals[arrival++];
            if (event.turn > start.turn) {
                if (state.n_shops == int(state.shops.size()) || event.type >= kag::N_SHOPS) std::abort();
                state.shops[state.n_shops++] = event.type;
            }
        }
        if (t != start.turn)
            apply(state.accounts[0], result.resources[0], calendar[t].before_market, rules.capacity);
        if (!forecast.rival_calendar.empty())
            apply(state.accounts[1], result.resources[1], forecast.rival_calendar[t].before_market, rules.capacity);
        if(trace)trace->resources_at(state,result,0);
        std::array<Orders, 2> orders;
        orders[0] = t == start.turn && first_orders ? *first_orders :
                    compact ? compact_orders(state.accounts[0], warm_orders[t], 1) : warm_orders[t];
        if (!forecast.rival_orders.empty()) orders[1] = forecast.rival_orders[t];
        for(int p=0;p<2;++p)if(repair_sides&(1<<p)) {
            const auto plan=p?forecast.rival_calendar:calendar;
            if(plan.empty())continue;
            PlannerObservation obs{t,state.accounts[p],result.resources[p],state.inventory,
                state.shops,state.n_shops,state.accounts[p^1].cash};
            orders[p]=repair_purchases(obs,plan,orders[p],rules,result.repairs[p]);
        }
        const auto traded = trade(state, orders, rules);
        if(trace) {
            trace->orders_at(state,0,calendar[t].commitments,orders[0],traded.accepted[0]);
            if(!forecast.rival_calendar.empty())trace->orders_at(state,1,forecast.rival_calendar[t].commitments,orders[1],traded.accepted[1]);
        }
        result.commitment_errors[0] += commitment_errors(calendar[t].commitments, orders[0], traded.accepted[0]);
        if (!forecast.rival_calendar.empty())
            result.commitment_errors[1] += commitment_errors(forecast.rival_calendar[t].commitments, orders[1], traded.accepted[1]);
        consume(state, rules);
        apply(state.accounts[0], result.resources[0], calendar[t].after_market, rules.capacity);
        if (!forecast.rival_calendar.empty())
            apply(state.accounts[1], result.resources[1], forecast.rival_calendar[t].after_market, rules.capacity);
        if(trace)trace->resources_at(state,result,1);
        advance(state, rules); ++result.evaluated_turns;
    }
    result.accounts = state.accounts;
    result.inventory = state.inventory; result.shops = state.shops;
    result.n_shops = state.n_shops; result.end_turn = state.turn;
    return result;
}

inline bool keeps_ending_resources(const ContinuationResult& candidate, const ContinuationResult& reference) {
    for (int item = 0; item < kag::N_ITEMS; ++item) {
        if (candidate.accounts[0].stock[item] < reference.accounts[0].stock[item]) return false;
        for (int b = 0; b < kag::MAX_UNITS; ++b)
            if (candidate.resources[0].buffers[b][item] < reference.resources[0].buffers[b][item]) return false;
    }
    for (int item = 0; item < kag::N_CROPS; ++item)
        if (candidate.accounts[0].seeds[item] < reference.accounts[0].seeds[item]) return false;
    return true;
}
}
