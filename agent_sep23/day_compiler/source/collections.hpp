#pragma once
#include "state.hpp"
#include "trading.hpp"
#include <algorithm>
#include <tuple>

namespace kag::day_compiler {
struct CollectionComparison {
    // Invalid comparisons are explicit: changed biology/assets, lost finished
    // stock, worsened finished quotes, or an infeasible input restoration.
    int rejection=0, extra_units=0;
    double restored_margin=0, score=0;
    bool valid() const { return rejection==0; }
};
inline bool same_collection_biology(const Tile& a,const Tile& b) {
    auto key=[](const Tile& t) {
        return std::tuple{t.kind,t.what,t.has_animal,t.watered_today,t.fed_today,t.cared_today,
            t.fertilizer_available,t.consecutive_dry,t.pending_care_bonus,t.planted_day,
            t.max_lifespan_step,t.fertilized_until_day};
    };
    const bool renewable=a.has_animal || (a.kind==T_PLANT && CROPS[a.what].ongoing);
    return key(a)==key(b) && (renewable?a.yield_units<=b.yield_units:a.yield_units==b.yield_units);
}
inline int unreachable_decay(const Sim& sim,int seat,int cell,const Tile& tile) {
    if(tile.kind!=T_PLANT || tile.max_lifespan_step<0 || !tile.yield_units)return 0;
    const auto& farm=sim.st.farms[seat];
    const int farmer=farm.pos_y[0]*BOARD+farm.pos_x[0];
    const int earliest=std::min(distance(farmer,cell),1+shed_distance(cell));
    const int expiry=tile.max_lifespan_step-sim.st.step;
    const int first_loss=expiry<0?(-expiry&1):expiry;
    // Harvest executes before that hour's decay. Even a free, immediately
    // hired worker cannot rescue units lost before its earliest arrival.
    return std::min(int(tile.yield_units),std::max(0,(earliest-first_loss+1)/2));
}
// A deliberately conservative first collection policy. Preserve every owned
// finished unit, count both field holders and shed stock, and value only extra
// production at the $1 floor. Existing field-to-shed transfers earn no dollars.
// Exact input restoration accounts for changes in wheat/fertilizer holdings.
// This is a development heuristic, not a calibrated general next-state value.
inline CollectionComparison compare_collections_impl(const Sim& candidate,const Sim& baseline,int seat,bool market_value) {
    CollectionComparison result;
    const auto& own=candidate.st.farms[seat]; const auto& target=baseline.st.farms[seat];
    if(candidate.st.day!=baseline.st.day || candidate.st.hour!=0 || baseline.st.hour!=0 ||
       own.n_quadrants!=target.n_quadrants || !std::equal(own.seeds,own.seeds+N_CROPS,target.seeds) ||
       !std::equal(own.shed+GOOSE,own.shed+N_ITEMS,target.shed+GOOSE)) {
        result.rejection=1; return result;
    }
    int available[N_PRODUCTS]{},required[N_PRODUCTS]{},unreachable[N_PRODUCTS]{},target_unreachable[N_PRODUCTS]{};
    std::copy_n(own.shed,N_PRODUCTS,available); std::copy_n(target.shed,N_PRODUCTS,required);
    for(int c=0;c<100;++c) {
        const auto& a=own.tiles[c/10][c%10]; const auto& b=target.tiles[c/10][c%10];
        if(!same_collection_biology(a,b)) { result.rejection=1; return result; }
        if(a.has_animal) {
            const int p=ANIMALS[a.what-GOOSE].product;
            available[p]+=a.yield_units; required[p]+=b.yield_units;
        } else if(a.kind==T_PLANT) {
            available[a.what]+=a.yield_units;required[b.what]+=b.yield_units;
            unreachable[a.what]+=unreachable_decay(candidate,seat,c,a);
            target_unreachable[b.what]+=unreachable_decay(baseline,seat,c,b);
        }
    }
    for(int p=CARROT;p<=WOOL;++p) {
        const int saved=available[p]-required[p]+own.sold_units[p]-target.sold_units[p];
        const int discarded=std::max(0,own.discarded[p]-target.discarded[p]);
        // Engine-confirmed overflow is an economic cost, not unexplained stock
        // destruction. The value below charges every lost unit; the old floor
        // comparison continues to require strict finished-stock preservation.
        if(market_value?saved+discarded<0:available[p]<required[p]) { result.rejection=2; return result; }
        if(!market_value && required[p] && candidate.st.market.inventory[p]>baseline.st.market.inventory[p]) {
            result.rejection=3; return result;
        }
        result.extra_units+=market_value?saved:available[p]-required[p];
    }
    if(market_value) {
        double liquidated=0,target_liquidated=0,input_restoration=0;
        for(int p=CARROT;p<=WOOL;++p) {
            liquidated+=transact(p,candidate.st.market.inventory[p],available[p]-unreachable[p],0).own_cash;
            target_liquidated+=transact(p,baseline.st.market.inventory[p],required[p]-target_unreachable[p],0).own_cash;
        }
        for(int p:{WHEAT,FERTILIZER})
            input_restoration+=transact(p,candidate.st.market.inventory[p],own.shed[p]-target.shed[p],0).own_cash;
        // Restore inputs after hypothetical finished-goods liquidation. Requiring
        // both inventories to fit simultaneously rejects valuable replacement
        // of cheap wheat by rescued fruit in a full shed. This is a value model;
        // today's schedule and its actual storage still require exact verification.
        result.restored_margin=own.money-target.money+input_restoration-
            candidate.st.farms[seat^1].money+baseline.st.farms[seat^1].money;
        result.score=result.restored_margin+liquidated-target_liquidated;
        if(own.money+liquidated+input_restoration<0)result.rejection=4;
        return result;
    }
    Action restore; restore.n_units=own.n_units;
    for(int p:{WHEAT,FERTILIZER})if(own.shed[p]>target.shed[p])
        restore.orders[restore.n_orders++]={M_SELL,uint8_t(p),own.shed[p]-target.shed[p]};
    for(int p:{WHEAT,FERTILIZER})if(own.shed[p]<target.shed[p])
        restore.orders[restore.n_orders++]={M_BUY_PRODUCT,uint8_t(p),target.shed[p]-own.shed[p]};
    restore.finalize(); auto restored=candidate;
    if(restore.n_orders) {
        const auto check=restored.diagnose_solo_action(seat,restore);
        if(check.requested_order_units!=check.successful_order_units) { result.rejection=4; return result; }
        if(seat)restored.step(Action{},restore);else restored.step(restore,Action{});
        for(int p:{WHEAT,FERTILIZER})if(restored.st.farms[seat].shed[p]!=target.shed[p]) {
            result.rejection=4; return result;
        }
    }
    result.restored_margin=restored.st.farms[seat].money-target.money-
        candidate.st.farms[seat^1].money+baseline.st.farms[seat^1].money;
    result.score=result.restored_margin;
    result.score+=result.extra_units;
    return result;
}
inline CollectionComparison compare_collections(const Sim& candidate,const Sim& baseline,int seat) {
    return compare_collections_impl(candidate,baseline,seat,false);
}
inline CollectionComparison compare_collection_value(const Sim& candidate,const Sim& baseline,int seat) {
    return compare_collections_impl(candidate,baseline,seat,true);
}
}
