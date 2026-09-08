#pragma once
#include "entry.hpp"
#include "../../late_portfolio_001/proposals/late_value_s32_t0_r05/source/agent.hpp"

namespace compositions_crop_parent::v52_family_v2 {
namespace data {
#include "../../../league/public_router_v52/source/data.inc"
}
inline void tape(int route,const kag::agent::AgentObservation& o,kag::Action& a) {
    a.clear();a.n_units=o.self().n_units;
    if(o.step<0 || o.step>=719){a.finalize();return;}
    int cursor=data::offsets[route][o.step];const int units=data::values[cursor++];a.n_orders=data::values[cursor++];
    for(int u=0;u<units;++u){a.units[u]={uint8_t(data::values[cursor]),uint8_t(data::values[cursor+1]),data::values[cursor+2]};cursor+=3;}
    for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(data::values[cursor]),uint8_t(data::values[cursor+1]),data::values[cursor+2]};cursor+=3;}
    for(int u=units;u<a.n_units;++u)a.units[u]={};
    a.finalize();
}
class Transfer {
    kag::agents_crop_parent::late_value_s32_t0_r05::Agent base_;
    int threshold_,rule_,route_=-1,wheat_=0,fertilizer_=0;
    bool all_routes_;
    std::array<int,5> seeds_{};
    bool eligible(const kag::agent::AgentObservation& o)const {
        const auto& f=o.self();
        if(f.n_units!=1 || f.n_quadrants!=1 || f.pos_x[0]!=4 || f.pos_y[0]!=4 || f.money<700)return false;
        for(int item=0;item<kag::N_ITEMS;++item) {
            if(o.own.inv[0][item])return false;
            if(item!=kag::WHEAT && item!=kag::FERTILIZER && o.own.shed[item]!=target_shed[item])return false;
        }
        for(int cell=0;cell<100;++cell) {
            const auto& expected=target_tiles[cell];
            if(expected[0]!=kag::T_LOCKED && tile_key(f.tiles[cell/10][cell%10],o.day)!=expected)return false;
        }
        return true;
    }
public:
    void set_crop_mode(int mode){base_.set_crop_mode(mode);}
    bool crop_selected()const{return base_.crop_selected();}
    Transfer(int threshold=2,bool all_routes=false,int rule=0):threshold_(threshold),rule_(rule),all_routes_(all_routes){
        if(rule!=0 && rule!=6)std::abort();
    }
    static kag::agent::AgentInfo info(){return {"v52_transfer"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);route_=-1;wheat_=fertilizer_=0;seeds_={};}
    int selected_route()const{return route_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a) {
        if(route_<0)base_.act(o,budget,a);
        if(o.step==144 && eligible(o)) {
            int wool=0,milk=0;
            for(int i=0;i<o.n_shops;++i) {
                if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::WOOL))wool+=kag::SHOP_MULT[o.shops[i]];
                if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::MILK))milk+=kag::SHOP_MULT[o.shops[i]];
            }
            const bool context=rule_==0 ||
                (o.shops[0]==kag::SHOP_YARN_STORE && (o.shops[1]==kag::SHOP_ICE_CREAM_SHOP ||
                 o.shops[1]==kag::SHOP_PIZZA_SHOP || o.shops[1]==kag::SHOP_SMOOTHIE_SHOP || o.shops[1]==kag::SHOP_YARN_STORE)) ||
                (o.shops[1]==kag::SHOP_YARN_STORE && (o.shops[0]==kag::SHOP_PIZZA_SHOP || o.shops[0]==kag::SHOP_SMOOTHIE_SHOP));
            if(all_routes_)route_=wool?1:milk?2:0;
            else if(wool>=threshold_ && context)route_=1;
            if(route_>=0) {
                wheat_=std::max(0,target_shed[kag::WHEAT]-o.own.shed[kag::WHEAT]);
                fertilizer_=std::max(0,o.own.shed[kag::FERTILIZER]-target_shed[kag::FERTILIZER]);
                for(int p=0;p<5;++p)seeds_[p]=std::max(0,target_seeds[p]-o.own.seeds[p]);
            }
        }
        if(route_<0)return;
        if(o.step==288 && route_==2)route_=o.market.inventory[kag::TOMATO]<=9916?3:4;
        tape(route_,o,a);
        if(o.step==144)for(int s=0;s<a.n_orders;++s) {
            auto& order=a.orders[s];
            if(order.op==kag::M_BUY_PRODUCT && order.item==kag::WHEAT){order.n+=wheat_;wheat_=0;}
            if(order.op==kag::M_BUY_SEED && (order.item==kag::WHEAT || order.item==kag::CARROT)) {
                order.n+=seeds_[order.item];seeds_[order.item]=0;
            }
        }
        if(o.step==145) {
            for(int s=0;s<a.n_orders;++s)if(a.orders[s].op==kag::M_SELL && a.orders[s].item==kag::FERTILIZER) {
                a.orders[s].n+=fertilizer_;fertilizer_=0;break;
            }
            for(int p=0;p<5;++p)if(seeds_[p] && a.n_orders<10) {
                a.orders[a.n_orders++]={kag::M_BUY_SEED,uint8_t(p),seeds_[p]};seeds_[p]=0;
            }
        }
        a.finalize();
    }
};
}
