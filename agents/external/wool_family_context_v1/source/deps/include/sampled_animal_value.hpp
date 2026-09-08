#pragma once
#include "animal_investment_value.hpp"

namespace catalog_wool_family_context_v1_compositions {
// Conditional sample of unknown shops, shared across all choices. Every block
// of eight samples contains each shop exactly once at each unknown reveal.
// There is no environment seed, future observation, or live simulator here.
// Quantity, intraday execution, and rival expansion remain heuristic.
class SampledAnimalValue {
    const kag::agent::AgentObservation& observation_;
    int count_;
    std::array<ProductFlows,64> demand_;
    static uint64_t random(uint64_t& state) {
        state+=0x9e3779b97f4a7c15ULL;uint64_t x=state;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
public:
    SampledAnimalValue(const kag::agent::AgentObservation& o,int count):observation_(o),count_(count) {
        if(count<0 || count>64 || (count!=1 && count%8))std::abort();
        // One denotes an undiscounted mean-demand control, so mean versus
        // sampled forecasts differ only in integration through the price curve.
        if(count==1) {
            for(int day=o.day;day<30;++day)for(int product=0;product<kag::N_PRODUCTS;++product) {
                double known=product==kag::FERTILIZER?0:1,expected=0;
                for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<product))known+=6*kag::SHOP_MULT[o.shops[i]];
                for(int shop=0;shop<kag::N_SHOPS;++shop)if(kag::SHOP_MASK[shop]&(1u<<product))expected+=6.*kag::SHOP_MULT[shop]/int(kag::N_SHOPS);
                demand_[0][day][product]=known+std::max(0,std::min(8,day/3)-o.n_shops)*expected;
            }
            return;
        }
        for(int sample=0;sample<count_;++sample) {
            std::array<int,8> shops;
            for(int reveal=0;reveal<8;++reveal) {
                if(reveal<o.n_shops){shops[reveal]=o.shops[reveal];continue;}
                std::array<int,8> permutation{0,1,2,3,4,5,6,7};
                uint64_t state=uint64_t(sample/8+1)*0xd1b54a32d192ed03ULL ^ uint64_t(reveal+1)*0x94d049bb133111ebULL;
                for(int i=7;i>0;--i)std::swap(permutation[i],permutation[random(state)%(i+1)]);
                shops[reveal]=permutation[sample%8];
            }
            for(int day=o.day;day<30;++day)for(int product=0;product<kag::N_PRODUCTS;++product) {
                double demand=product==kag::FERTILIZER?0:1;
                for(int reveal=0;reveal<std::min(8,day/3);++reveal)
                    if(kag::SHOP_MASK[shops[reveal]]&(1u<<product))demand+=6*kag::SHOP_MULT[shops[reveal]];
                demand_[sample][day][product]=demand;
            }
        }
    }
    InvestmentValue value(const FarmFlowPlan& flows,const Biology& addition,double operation_cost=0) const {
        if(!count_)return value_animal_investment(observation_,flows,addition,operation_cost);
        auto result=value_animal_investment(observation_,flows,addition,operation_cost,1,&demand_[0]);
        for(int sample=1;sample<count_;++sample) {
            const auto next=value_animal_investment(observation_,flows,addition,operation_cost,1,&demand_[sample]);
            result.own+=next.own;result.rival+=next.rival;
        }
        result.own/=count_;result.rival/=count_;return result;
    }
};
}
