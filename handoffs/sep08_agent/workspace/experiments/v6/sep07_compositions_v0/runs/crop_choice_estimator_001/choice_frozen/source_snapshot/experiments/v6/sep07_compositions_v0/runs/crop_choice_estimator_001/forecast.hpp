#pragma once
#include "../public_crop_forecast_001/source/crop_forecast.hpp"
#include "../../include/herd_forecast.hpp"

namespace compositions::crop_choice {
using public_crop_forecast::Days;
using public_crop_forecast::Products;
struct Plan {Days sales{},buys{};std::array<double,30> fixed{};};
struct Value {
    double own=0,rival=0;
    Products own_product{},rival_product{};
    double margin()const{return own-rival;}
};

// Full productive service and earliest capped harvest, matching the current
// crop helper. This only identifies when a hypothetical successor may start.
inline int current_harvest(const kag::agent::AgentObservation& o,const kag::Tile& tile) {
    const auto& crop=kag::CROPS[tile.what];
    if(crop.ongoing)return tile.planted_day+crop.first_yield_day+(crop.max_yield-1)*crop.interval;
    int held=tile.yield_units;
    const int last=std::min(29,std::max(o.day,tile.planted_day+crop.max_yield_day));
    for(int day=o.day;day<=last;++day) {
        const int age=day-tile.planted_day;
        if((day>o.day||!tile.watered_today)&&age>=(crop.max_yield_day+1)/2&&age<=crop.max_yield_day)
            held=std::min(crop.max_yield,held+2);
        if(age>=crop.first_yield_day&&(held>=crop.max_yield||day==last))return day;
    }
    return 30;
}

// Mode0: visible animals only. Mode1 adds current crops. Mode2 repeats observed
// wheat/carrot families. Mode3 also uses a terminal carrot after wheat. These
// are explicit uncertain policies; later expansion and private stocks are absent.
inline Days rival_flows(const kag::agent::AgentObservation& o,int mode,bool subtract_feed,bool animal_fertilizer) {
    using namespace kag;
    Days result{};
    const auto animals=forecast_herds(o);
    for(int day=o.day;day<30;++day)for(int product=EGG;product<=WOOL;++product)
        result[day][product]=animals[1][day][product-EGG];
    if(mode>0) {
        const auto crops=public_crop_forecast::crop_output(o,public_crop_forecast::Mode::cap_fully_fertilized);
        for(int day=o.day;day<30;++day)for(int product=0;product<N_PRODUCTS;++product)result[day][product]+=crops.output[day][product];
    }
    for(const auto& row:o.opponent().tiles)for(const auto& tile:row) {
        if(tile.has_animal) {
            result[o.day][ANIMALS[tile.what-GOOSE].product]+=tile.yield_units;
            if(subtract_feed) {
                result[o.day][WHEAT]-=!tile.fed_today;
                for(int day=o.day+1;day<30;++day)--result[day][WHEAT];
            }
            if(animal_fertilizer) {
                result[o.day][FERTILIZER]+=tile.fertilizer_available;
                for(int day=o.day+1;day<30;++day)++result[day][FERTILIZER];
            }
        }
        if(mode<2||tile.kind!=T_PLANT||(tile.what!=WHEAT&&tile.what!=CARROT))continue;
        int start=current_harvest(o,tile),item=tile.what;
        while(start<29) {
            if(start+CROPS[item].max_yield_day>29) {
                if(mode<3||item!=WHEAT||start+CROPS[CARROT].max_yield_day>29)break;
                item=CARROT;
            }
            const int end=start+CROPS[item].max_yield_day;
            result[end][item]+=CROPS[item].max_yield;
            // One dose covers the productive days of these short crops.
            --result[start+(CROPS[item].max_yield_day+1)/2][FERTILIZER];
            start=end;
        }
    }
    return result;
}

// Adapted from public_crop_forecast_001/source/branch_forecast.cpp. Quantities
// net within a day and quote at midpoint inventory. This is neither a funding
// certificate nor exact intraday market execution; compare against exact games.
inline Value value(const kag::agent::AgentObservation& o,const Plan& own,const Days& rival,const Days& demand) {
    Value result;result.own=o.self().money;result.rival=o.opponent().money;
    Products inventory{};for(int product=0;product<kag::N_PRODUCTS;++product)inventory[product]=o.market.inventory[product];
    for(int day=o.day;day<30;++day) {
        result.own-=own.fixed[day];
        for(int product=0;product<kag::N_PRODUCTS;++product) {
            double sell=own.sales[day][product],buy=own.buys[day][product];
            const double internal=std::min(sell,buy);sell-=internal;buy-=internal;
            const double other_sell=std::max(0.,rival[day][product]),other_buy=std::max(0.,-rival[day][product]);
            const double mid=inventory[product]+.5*(sell+other_sell-buy-other_buy-demand[day][product]);
            const int price=fractional_quote(product,mid),buy_price=fractional_quote(product,mid-1);
            const double income=sell*price-buy*buy_price,other_income=other_sell*price-other_buy*buy_price;
            result.own+=income;result.rival+=other_income;
            result.own_product[product]+=income;result.rival_product[product]+=other_income;
            inventory[product]-=demand[day][product]+buy+other_buy;
            if(price>1)inventory[product]+=sell+other_sell;
        }
    }
    return result;
}
}
