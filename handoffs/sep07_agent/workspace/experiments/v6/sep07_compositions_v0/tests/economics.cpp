#include "../include/economics.hpp"
#include <iostream>

using namespace kag;
using namespace compositions;

bool verify(int test) {
    FinancialPlan plan;
    EconomicScenario scenario;
    for(int i=0;i<8;++i) scenario.shops[i]=(i*3)%N_SHOPS;
    if(test==0) {
        for(int day=0;day<30;++day) plan.use[day*24+1][WHEAT]=5;
        plan.arrivals[12][MILK]=50;
    } else if(test==1) {
        for(int step=0;step<8;++step) {
            plan.arrivals[step][FERTILIZER]=100;
            scenario.rival_sells[step][FERTILIZER]=100;
        }
    } else if(test==2) plan.arrivals[8][MILK]=150;
    else if(test==3) {
        plan.fixed_cost[0]=5000;
        plan.arrivals[100][MELON]=20;
    } else {
        scenario.rival_fixed_cost=321;
        for(int step=0;step<20;++step)scenario.rival_buys[step][WHEAT]=2;
        for(int step=20;step<40;++step)scenario.rival_sells[step][WHEAT]=2;
    }
    const auto estimate=economics(plan,scenario);
    Config cfg;cfg.weed_chance=0;Sim sim(cfg);
    sim.st.farms[1].money-=scenario.rival_fixed_cost;
    for(int step=0;step<turns;++step) {
        std::copy_n(scenario.shops.begin(),sim.st.n_shops,sim.st.shops);
        auto& me=sim.st.farms[0];auto& rival=sim.st.farms[1];
        Action a,b;a.clear();b.clear();
        // Fixed external capital and input withdrawal isolate the market
        // contract. They are not a claim that a field schedule is executable.
        me.money-=plan.fixed_cost[step];
        if(test==0 && step%24==1) {me.shed[WHEAT]-=5;me.shed_total-=5;}
        if(test==0 && (step==0 || (step%24==1 && step<29*24)))
            a.orders[a.n_orders++]={M_BUY_PRODUCT,WHEAT,5};
        for(int item=0;item<N_PRODUCTS;++item) {
            const int arrival=plan.arrivals[step][item];
            if(arrival) {
                me.inv_add(0,item,arrival);
                a.units[0]={OP_DROP,0,1};
                a.orders[a.n_orders++]={M_SELL,uint8_t(item),std::min(arrival,100)};
            }
            const int sale=scenario.rival_sells[step][item];
            if(sale) {
                if(test!=4) {rival.inv_add(0,item,sale);b.units[0]={OP_DROP,0,1};}
                b.orders[b.n_orders++]={M_SELL,uint8_t(item),sale};
            }
            const int buy=scenario.rival_buys[step][item];
            if(buy)b.orders[b.n_orders++]={M_BUY_PRODUCT,uint8_t(item),buy};
        }
        a.finalize();b.finalize();sim.step(a,b);
    }
    const auto& me=sim.st.farms[0];
    bool ok=me.money==estimate.cash && sim.st.farms[1].money==estimate.rival_cash;
    for(int item=0;item<N_PRODUCTS;++item)
        ok &= sim.st.market.inventory[item]==estimate.market[item] && me.shed[item]==estimate.residue[item]
           && me.sold_units[item]==estimate.sold[item] && me.discarded[item]==estimate.discarded[item];
    if(test==3) ok &= estimate.first_funding_step==0 && estimate.min_cash==-2000;
    if(!ok) std::cerr<<"case="<<test<<" exact_cash="<<me.money<<" estimated_cash="<<estimate.cash<<'\n';
    return ok;
}

int main() {
    for(int test=0;test<5;++test) if(!verify(test)) return 1;
    std::cout<<"economic_market_cases=5 full_turns_each=719 both_cash_values_checked\n";
}
