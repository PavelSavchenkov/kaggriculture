#pragma once
#include "biology.hpp"
#include "../../../../../../../../../../agents/common/api/agent_api.hpp"
#include <algorithm>
#include <cmath>

namespace compositions_crop_parent {
using HerdOutput=std::array<std::array<double,3>,30>;
struct HerdForecast {
    double own=0,rival=0;
    double margin() const {return own-rival;}
};

// Adapted from Dmitrii Gluzdov's public S38 portfolio projection. Existing
// public herds receive full daily feed/care and daily sales; unknown shops get
// a discounted expected demand. New animal output instead uses our explicit
// dated biology/service contract. This is a heuristic, not a feasibility bound.
inline void forecast_animal(HerdOutput& out,int item,int placed,int pending,int day) {
    const auto& def=kag::ANIMALS[item-kag::GOOSE];
    for(int future=day+1;future<30;++future) {
        if(future>=placed+def.first_yield_day && (future-placed-def.first_yield_day)%def.interval==0) {
            out[future][def.product-kag::EGG]+=std::min(def.max_held,1+pending);pending=0;
        }
        ++pending;
    }
}

inline std::array<HerdOutput,2> forecast_herds(const kag::agent::AgentObservation& o) {
    std::array<HerdOutput,2> result{};
    for(int player=0;player<2;++player)for(const auto& row:o.farms[player].tiles)for(const auto& tile:row)
        if(tile.has_animal)forecast_animal(result[player==o.player?0:1],tile.what,tile.planted_day,tile.pending_care_bonus,o.day);
    for(int item=kag::GOOSE;item<=kag::SHEEP;++item) {
        int count=o.own.shed[item];for(int u=0;u<o.self().n_units;++u)count+=o.own.inv[u][item];
        for(int n=0;n<count;++n)forecast_animal(result[0],item,o.day+1,0,o.day);
    }
    return result;
}

inline int fractional_quote(int item,double inventory) {
    const auto& p=kag::MARKET[item];const bool below=inventory<p.I0;
    const double distance=std::abs(inventory-p.I0),target=below?p.below_t:p.above_t;
    const auto curve=below?p.below_f:p.above_f;
    const double price=p.base+(below?1.:-1.)*target*p.base*kag::shape(curve,distance,p.T)/kag::shape(curve,p.T,p.T);
    return std::max(1,int(std::nearbyint(price)));
}

inline HerdForecast value_herd_choice(const kag::agent::AgentObservation& o,const std::array<HerdOutput,2>& herds,
                                      const Biology& addition,double future_weight=.35) {
    HerdForecast result;
    for(int product=kag::EGG;product<=kag::WOOL;++product) {
        double stock=o.market.inventory[product],daily=1,mean_shop=0;
        for(int k=0;k<o.n_shops;++k)if(kag::SHOP_MASK[o.shops[k]]&(1u<<product))daily+=6*kag::SHOP_MULT[o.shops[k]];
        for(int k=0;k<kag::N_SHOPS;++k)if(kag::SHOP_MASK[k]&(1u<<product))mean_shop+=6.*kag::SHOP_MULT[k]/int(kag::N_SHOPS);
        for(int day=o.day+1;day<30;++day) {
            const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
            const double demand=daily+future_weight*unseen*mean_shop;
            const double own=herds[0][day][product-kag::EGG]+addition.days[day].output[product];
            const double rival=herds[1][day][product-kag::EGG];
            const int quote=fractional_quote(product,stock+.5*(own+rival)-.5*demand);
            result.own+=own*quote;result.rival+=rival*quote;
            stock-=demand;if(quote>1)stock+=own+rival;
        }
    }
    result.own-=addition.animal_cost;
    // Original service route stays fixed. Its equal wheat/operation obligations
    // cancel between choices; differing future route costs are not estimated.
    return result;
}
}
