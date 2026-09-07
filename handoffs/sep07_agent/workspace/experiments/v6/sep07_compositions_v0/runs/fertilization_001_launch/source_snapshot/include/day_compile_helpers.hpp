#pragma once
#include "day_contract.hpp"
#include "guarded_day.hpp"
#include "market_tape.hpp"

namespace compositions::day_contract {
namespace fs=std::filesystem;
inline GuardedDay guarded(const RecordedDay& source,const DayProblem& problem,const std::array<Action,24>& actions) {
    GuardedDay result;result.plan={source.start.st.day,actions};
    const auto& farm=source.start.st.farms[0];result.quadrants=farm.n_quadrants;
    std::copy_n(farm.shed,N_ITEMS,result.shed.begin());std::copy_n(farm.seeds,N_CROPS,result.seeds.begin());
    for(int cell=0;cell<100;++cell) {
        const auto& t=farm.tiles[cell/10][cell%10];result.tiles[cell]=tile_key(t,source.start.st.day);
        result.check[cell]=t.kind==T_PLANT || t.kind==T_COOP || t.kind==T_PASTURE;
    }
    for(const auto& work:problem.tile_work)result.check[work.tile]=true;
    return result;
}

// Offline funding check: keep every previously accepted market quantity and
// fixed purchase while inserting the new obligation at the earliest safe hour.
// A day solver's stock constraints alone cannot prove that purchases are paid.
inline bool insert_funded_order(const RecordedDay& source,DayProblem& problem,std::array<Action,24>& markets,
                         const std::array<uint8_t,8>& shops,int op,int product,int quantity,
                         int reserved_hour=-1,int reserved_slot=-1) {
    auto accepted=[&](const std::array<Action,24>& actions) {
        auto sim=source.start;std::array<MarketStep,24> result;
        for(int hour=0;hour<24;++hour) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            Action pair[2]={actions[hour],source.rival[hour]};result[hour]=accepted_market(sim,pair);sim.step(pair[0],pair[1]);
        }
        return result;
    };
    const auto baseline=accepted(markets);
    for(int hour=0;hour<23;++hour) {
        int slot=markets[hour].n_orders;
        if(hour==reserved_hour && reserved_slot>=0 && reserved_slot<slot && markets[hour].orders[reserved_slot].op==M_NONE)slot=reserved_slot;
        if(slot>=10)continue;
        auto trial=markets;trial[hour].orders[slot]={uint8_t(op),uint8_t(product),quantity};
        trial[hour].n_orders=std::max(trial[hour].n_orders,slot+1);trial[hour].finalize();
        const auto outcome=accepted(trial);const auto& added=outcome[hour].slots[slot];
        bool paid=op==M_BUY_PRODUCT?added.trades[0].n==quantity:
            op==M_BUY_ANIMAL?added.fixed[0]==ANIMALS[product-GOOSE].cost*quantity:added.fixed[0]>0;
        for(int h=0;h<24 && paid;++h)for(int i=0;i<markets[h].n_orders && paid;++i) {
            if(h==hour && i==slot)continue;
            const auto& expected=baseline[h].slots[i];const auto& actual=outcome[h].slots[i];
            paid=expected.trades[0].n==actual.trades[0].n && expected.fixed[0]==actual.fixed[0];
        }
        if(!paid)continue;
        markets=std::move(trial);problem.market_plan.push_back({int8_t(hour),int8_t(slot),uint8_t(op),int16_t(op==M_HIRE?-1:product),quantity});
        return true;
    }
    return false;
}
inline void export_schedule(const std::string& name,const std::array<Action,24>& actions,const fs::path& folder) {
    std::ofstream header(folder/"schedule.hpp");
    header<<"#pragma once\n#include \"agents/common/api/agent_api.hpp\"\n#include <array>\nnamespace compositions::"<<name
        <<" {inline std::array<kag::Action,24> schedule(){constexpr int data[]={\n";
    for(const auto& action:actions) {
        header<<action.n_units<<','<<action.n_orders<<',';
        for(int u=0;u<action.n_units;++u)header<<+action.units[u].op<<','<<+action.units[u].arg<<','<<action.units[u].n<<',';
        for(int i=0;i<action.n_orders;++i)header<<+action.orders[i].op<<','<<+action.orders[i].item<<','<<action.orders[i].n<<',';
        header<<'\n';
    }
    header<<"};std::array<kag::Action,24> result;const int* p=data;\nfor(auto& action:result){action.n_units=*p++;action.n_orders=*p++;"
        <<"for(int u=0;u<action.n_units;++u){action.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}"
        <<"for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}action.finalize();}return result;}}\n";
}

}
