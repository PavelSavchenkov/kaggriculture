#pragma once
#include "herd_forecast.hpp"

namespace catalog_animal_repair_premium_m2_sale {
using ProductFlows=std::array<std::array<double,kag::N_PRODUCTS>,30>;
struct FarmFlowPlan {
    ProductFlows sales{},buys{};
};
struct InvestmentValue {
    double own=0,rival=0,service_cost=0,animal_cost=0;
    int first_output_day=30,wheat=0,fertilizer=0,operations=0;
    double margin() const {return own-rival;}
};

// Daily market approximation over the whole remaining farm. The immutable
// baseline contains intended own trade quantities, never future market quotes.
// Unknown rival expansion, intraday funding, storage, and service route costs
// remain explicit heuristic gaps; this is not a feasibility certificate.
inline InvestmentValue value_animal_investment(
        const kag::agent::AgentObservation& o,const FarmFlowPlan& baseline,
        const Biology& addition,double operation_cost=0,double future_weight=.35,
        const ProductFlows* scenario_demand=nullptr) {
    InvestmentValue value;const auto herds=forecast_herds(o);
    std::array<double,kag::N_PRODUCTS> stock{},known{},expected{};
    for(int product=0;product<kag::N_PRODUCTS;++product) {
        stock[product]=o.market.inventory[product];known[product]=1;
        for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<product))known[product]+=6*kag::SHOP_MULT[o.shops[i]];
        for(int shop=0;shop<kag::N_SHOPS;++shop)if(kag::SHOP_MASK[shop]&(1u<<product))expected[product]+=6.*kag::SHOP_MULT[shop]/int(kag::N_SHOPS);
    }
    for(int day=o.day;day<30;++day) {
        const auto& biology=addition.days[day];
        value.wheat+=biology.wheat;value.fertilizer+=biology.output[kag::FERTILIZER];value.operations+=biology.operations;
        for(int product=kag::EGG;product<=kag::WOOL;++product)
            if(biology.output[product])value.first_output_day=std::min(value.first_output_day,day);
        for(int product=0;product<kag::N_PRODUCTS;++product) {
            const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
            const double demand=scenario_demand?(*scenario_demand)[day][product]:known[product]+future_weight*unseen*expected[product];
            double sells=baseline.sales[day][product]+biology.output[product];
            double buys=baseline.buys[day][product]+(product==kag::WHEAT?biology.wheat:0);
            // Feed can use planned wheat sales, fertilizer can replace planned
            // purchases. Only net additional market pressure remains.
            const double internal=std::min(sells,buys);sells-=internal;buys-=internal;
            const double rival=product>=kag::EGG && product<=kag::WOOL?herds[1][day][product-kag::EGG]:0;
            const double mid=stock[product]+.5*(sells+rival-buys-demand);
            const int sell_price=fractional_quote(product,mid);
            const int buy_price=fractional_quote(product,mid-1);
            value.own+=sells*sell_price-buys*buy_price;value.rival+=rival*sell_price;
            stock[product]-=demand+buys;if(sell_price>1)stock[product]+=sells+rival;
        }
    }
    value.animal_cost=addition.animal_cost;value.service_cost=operation_cost*value.operations;
    value.own-=value.animal_cost+value.service_cost;return value;
}

inline Biology investment_biology(int item,int day,Service service,bool force_entry_service=true) {
    if(!kag::is_animal(item) || day<0 || day>=30)std::abort();
    if(force_entry_service){service.feed|=uint32_t(1)<<day;service.care|=uint32_t(1)<<day;}
    return biology({uint8_t(item),1,day,30},service);
}
}
