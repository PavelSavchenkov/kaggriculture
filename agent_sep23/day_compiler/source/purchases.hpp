#pragma once
#include "state.hpp"
#include <algorithm>

namespace kag::day_compiler {
// Retain the worker program and every remaining seed quantity, but buy seeds
// by their actual planting deadlines. Full-engine funding verification follows.
inline bool delay_seed_purchases(const Observation& o,const Action* previous,Action* result,
                                int hours,const Configuration& config={}) {
    if(o.hour<0 || hours<=o.hour || hours>24)return false;
    int total[N_CROPS]{},placed[N_CROPS]{};
    for(int h=0;h<24;++h) {
        result[h]=previous[h];
        if(h<o.hour || h>=hours)continue;
        result[h].n_orders=0;
        for(int k=0;k<previous[h].n_orders;++k) {
            const auto x=previous[h].orders[k];
            if(x.op==M_BUY_SEED && x.item<N_CROPS)total[x.item]+=x.n;
            else if(x.op!=M_NONE)result[h].orders[result[h].n_orders++]=x;
        }
    }
    auto place=[&](int product,int quantity,int deadline) {
        if(quantity<=0)return true;
        if(placed[product]+quantity>total[product])return false;
        for(int h=deadline;h>=o.hour;--h) {
            for(int k=0;k<result[h].n_orders;++k) {
                auto& x=result[h].orders[k];
                if(x.op==M_BUY_SEED && x.item==product) {
                    x.n+=quantity; placed[product]+=quantity; return true;
                }
            }
            if(result[h].n_orders<config.max_orders) {
                result[h].orders[result[h].n_orders++]={M_BUY_SEED,uint8_t(product),quantity};
                placed[product]+=quantity; return true;
            }
        }
        return false;
    };
    int available[N_CROPS]; std::copy_n(o.own.seeds,N_CROPS,available);
    for(int h=o.hour;h<hours;++h) {
        int need[N_CROPS]{};
        for(int u=0;u<previous[h].n_units;++u) {
            const auto x=previous[h].units[u];
            if(x.op==OP_PLANT && x.arg<N_CROPS)++need[x.arg];
        }
        for(int p=0;p<N_CROPS;++p) {
            const int bought=std::max(0,need[p]-available[p]);
            if(!place(p,bought,h-1))return false;
            available[p]+=bought-need[p];
        }
    }
    for(int p=0;p<N_CROPS;++p)if(!place(p,total[p]-placed[p],hours-1))return false;
    for(int h=o.hour;h<hours;++h)result[h].finalize();
    return true;
}
}
