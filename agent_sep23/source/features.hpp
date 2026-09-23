#pragma once
#include "source/intent.hpp"
#include "source/history.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace bc {
namespace dc = kag::day_compiler;
using namespace kag;
constexpr int GLOBAL = 48, PRODUCT = 32, BOARD_FEATURES = 36, GROUP = 24, OPTION = 12;
struct CropFeatures {
    float key[GROUP]{};
    float options[dc::MAX_CROP_OPTIONS][OPTION]{};
    int16_t size=0, option_count=0, maximum[dc::MAX_CROP_OPTIONS]{};
};
struct AnimalFeatures { float key[GROUP]{}; int16_t size=0, species=0, forced=0; };
struct Features {
    float global[GLOBAL]{};
    float products[N_PRODUCTS][PRODUCT]{};
    float board[2][100][BOARD_FEATURES]{};
    std::array<CropFeatures,100> crops{};
    std::array<AnimalFeatures,100> animals{};
    int crop_count=0, animal_count=0, day=0, quadrants=0;
    float trajectory[150]{};
    bool trajectory_valid=false;
    float memory[30][50]{};
    bool memory_valid=false;
};
inline float scaled_log(double x) { return std::copysign(std::log1p(std::abs(x)),x)/10.; }
inline void geometry(const std::array<uint8_t,100>& cells,int count,float* out) {
    if(!count)return;
    float sum=0,maximum=0,min_x=9,min_y=9,max_x=0,max_y=0;
    for(int i=0;i<count;++i) {
        const int cell=cells[i]; const float d=dc::shed_distance(cell);
        sum+=d; maximum=std::max(maximum,d);
        min_x=std::min(min_x,float(cell%10));max_x=std::max(max_x,float(cell%10));
        min_y=std::min(min_y,float(cell/10));max_y=std::max(max_y,float(cell/10));
    }
    out[0]=sum/count/10;out[1]=maximum/10;out[2]=min_x/9;out[3]=min_y/9;
    out[4]=max_x/9;out[5]=max_y/9;
}
inline int producer(const Tile& tile) {
    if(tile.has_animal)return ANIMALS[tile.what-GOOSE].product;
    return tile.kind==T_PLANT?int(tile.what):-1;
}
inline Features encode(const dc::Observation& o,const dc::History& history,const dc::IntentSchema& schema,
                       const dc::Configuration& config={}) {
    Features f; f.crop_count=schema.crop_group_count;f.animal_count=schema.animal_group_count;
    f.day=o.day;f.quadrants=o.self().n_quadrants;
    auto& g=f.global;
    g[0]=o.day/29.f;g[1]=(29-o.day)/29.f;g[2]=o.day==29;g[3]=o.step/719.f;
    g[4]=scaled_log(o.self().money);g[5]=scaled_log(o.opponent().money);
    g[6]=o.self().n_quadrants/4.f;g[7]=o.opponent().n_quadrants/4.f;
    g[8]=o.self().n_units/16.f;g[9]=o.opponent().n_units/16.f;
    g[10]=o.own.shed_total/100.f;g[11]=(config.shed_capacity-o.own.shed_total)/100.f;
    for(int s=0;s<o.n_shops;++s)g[12+o.shops[s]]+=.125f;
    g[20]=(3-o.day%3)/3.f;g[21]=std::max(0,24-o.day)/24.f;
    for(int i=0;i<N_ITEMS;++i)g[24+i]=o.own.shed[i]/100.f;
    for(int i=0;i<N_CROPS;++i)g[36+i]=o.own.seeds[i]/100.f;
    g[41]=o.self().pos_x[0]/9.f;g[42]=o.self().pos_y[0]/9.f;
    g[43]=config.weed_chance*100;g[44]=config.hire_mult;
    for(int p=0;p<N_PRODUCTS;++p) {
        auto& v=f.products[p];v[0]=scaled_log(o.market.prices[p]);
        v[1]=(o.market.inventory[p]-10000)/100.f;v[2]=o.own.shed[p]/100.f;
        for(int u=0;u<o.self().n_units;++u)v[3]+=o.own.inv[u][p]/100.f;
        if(p<N_CROPS)v[4]=o.own.seeds[p]/100.f;
        for(int h=0;h<24;++h)v[5]+=dc::demand(o,config,p,o.step+h)/24.f;
        int valid=0;
        for(const auto& sample:history.samples())if(sample.step>=0 && sample.step<o.step && sample.step>=o.step-24) {
            ++valid;v[24]+=sample.lower[p]/100.f;v[25]+=sample.upper[p]/100.f;
            v[26]+=sample.identifiable[p]/24.f;v[27]+=sample.own_net_sales[p]/100.f;
            v[28]+=sample.own_fill_known[p]/24.f;v[29]+=sample.rival_visible_removal[p]/100.f;
            if(sample.step>=o.step-4)v[30]+=sample.lower[p]/100.f;
        }
        v[31]=valid/24.f;
    }
    for(int side=0;side<2;++side) {
        const auto& farm=side?o.opponent():o.self();
        for(int cell=0;cell<100;++cell) {
            const auto& t=farm.tiles[cell/10][cell%10];auto& b=f.board[side][cell];
            b[t.kind]=1; if(t.kind==T_PLANT||t.has_animal)b[6+t.what]=1;
            b[18]=t.has_animal;b[19]=(o.day-t.planted_day)/30.f;
            if(t.kind!=T_PLANT&&!t.has_animal)b[19]=0;
            b[20]=t.consecutive_dry/2.f;b[21]=t.yield_units/6.f;
            b[22]=t.kind==T_PLANT?std::clamp(t.fertilized_until_day-o.day,-1,3)/3.f:0;
            b[23]=t.pending_care_bonus/6.f;b[24]=t.watered_today;b[25]=t.fed_today;
            b[26]=t.cared_today;b[27]=t.fertilizer_available;
            b[28]=t.max_lifespan_step<0?-1:std::clamp(t.max_lifespan_step-o.step,-24,720)/720.f;
            b[29]=(cell%10)/9.f;b[30]=(cell/10)/9.f;b[31]=dc::shed_distance(cell)/10.f;
            b[32]=farm.pos_x[0]==cell%10 && farm.pos_y[0]==cell/10;
            b[33]=t.kind!=T_LOCKED;b[34]=t.kind==T_EMPTY||t.kind==T_WEED;
            const int p=producer(t); if(p<0)continue;
            auto& v=f.products[p];const int base=side?15:6;
            v[base]+=.01f;v[base+1]+=t.yield_units/100.f;
            v[base+2]+=(o.day-t.planted_day)/3000.f;
            v[base+3]+=t.consecutive_dry/100.f;v[base+4]+=dc::shed_distance(cell)/1000.f;
            v[base+5]+=t.fertilizer_available/100.f;
            const int age=o.day-t.planted_day;
            for(int horizon=1;horizon<=3;++horizon) {
                bool due=false;
                if(t.has_animal) { const auto& a=ANIMALS[t.what-GOOSE];due=age+horizon>=a.first_yield_day && (age+horizon-a.first_yield_day)%a.interval==0; }
                else {const auto& c=CROPS[t.what];due=c.ongoing?age+horizon>=c.first_yield_day && (age+horizon-c.first_yield_day)%c.interval==0 && age+horizon<=c.first_yield_day+c.interval*(c.max_yield-1):age+horizon==c.first_yield_day;}
                v[base+5+horizon]+=due/100.f;
            }
        }
    }
    for(int i=0;i<f.crop_count;++i) {
        const auto& c=schema.crops[i];auto& target=f.crops[i];auto& k=target.key;
        target.size=c.count;target.option_count=c.option_count;k[c.state.product]=1;
        k[5]=c.state.age/30.f;k[6]=c.state.held/6.f;k[7]=c.state.dry/2.f;
        k[8]=std::clamp<int>(c.state.fertilizer_expiry,-1,3)/3.f;
        k[9]=c.state.decay_expiry==-32768?-1:std::clamp<int>(c.state.decay_expiry,-24,720)/720.f;
        k[10]=c.state.production_phase/3.f;k[11]=c.state.watered_today;k[12]=c.count/100.f;
        geometry(c.cells,c.count,k+13);
        for(int j=0;j<c.option_count;++j) {
            const auto& procedure=c.options[j];auto& v=target.options[j];target.maximum[j]=procedure.maximum_count;
            v[int(procedure.goal.mode)]=1;v[4]=procedure.goal.harvest_age/30.f;
            v[5]=procedure.goal.min_yield/6.f;v[6]=procedure.water_actions/30.f;
            v[7]=procedure.fertilizer_actions/10.f;v[8]=procedure.maximum_count/float(c.count);
            v[9]=procedure.goal.mode==dc::CropMode::Yield && procedure.goal.harvest_age<=c.state.age;
            v[10]=c.option_count/32.f;v[11]=j/32.f;
        }
    }
    for(int i=0;i<f.animal_count;++i) {
        const auto& a=schema.animals[i];auto& target=f.animals[i];auto& k=target.key;
        target.size=a.count;target.species=a.state.species;
        target.forced=o.day<29 && a.state.unfed>=1 && !a.state.fed_today;
        k[a.state.species]=1;k[3]=a.state.age/30.f;k[4]=a.state.production_phase/3.f;
        k[5]=a.state.held/6.f;k[6]=a.state.unfed/2.f;k[7]=a.state.care_bank/6.f;
        k[8]=a.state.structure/5.f;k[9]=a.state.fed_today;k[10]=a.state.cared_today;
        k[11]=a.state.fertilizer_available;k[12]=a.count/100.f;geometry(a.cells,a.count,k+13);
        const auto& tile=o.self().tiles[a.cells[0]/10][a.cells[0]%10];
        k[19]=dc::zero_value_care(tile,o.day);k[20]=(dc::next_animal_production(tile,o.day)-o.day)/10.f;
    }
    return f;
}
}
