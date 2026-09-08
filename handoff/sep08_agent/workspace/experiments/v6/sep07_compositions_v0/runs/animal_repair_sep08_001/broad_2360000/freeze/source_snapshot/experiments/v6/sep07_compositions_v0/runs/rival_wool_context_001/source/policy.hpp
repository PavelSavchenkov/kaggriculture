#pragma once
#include "../../v52_family_001/source/improved.hpp"
#include "../../wool_family_context_v2_001/proposals/wool_family_context_v2/source/agent.hpp"

namespace compositions::rival_wool_context {
class Policy {
    kag::agents::wool_family_context_v2::Agent base_;
    v52_family::OptimizedTransfer wool_{0};
    kag::agent::AgentConfig config_;
    std::array<kag::Order,4> deferred_{};
    int mode_,first_=0,second_=0,farmer_=0,old_wheat_=0,rival_wheat_flow_=0;
    bool pending_=false,active_=false;
    static bool v2_context(int first,int second) {
        return (first==kag::SHOP_YARN_STORE && (second==kag::SHOP_ICE_CREAM_SHOP ||
                second==kag::SHOP_PIZZA_SHOP || second==kag::SHOP_SMOOTHIE_SHOP || second==kag::SHOP_YARN_STORE)) ||
               (second==kag::SHOP_YARN_STORE && (first==kag::SHOP_PIZZA_SHOP || first==kag::SHOP_SMOOTHIE_SHOP));
    }
public:
    // 0: force a compatible transfer; 1: unchanged v2 contexts; 2: broaden
    // for an observed early wheat purchase with the six-hire wool prefix.
    explicit Policy(int mode=1):mode_(mode){if(mode<0 || mode>2)std::abort();}
    static kag::agent::AgentInfo info(){return {"family_delay"};}
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);wool_.reset(init);config_=init.config;deferred_={};
        first_=second_=farmer_=old_wheat_=rival_wheat_flow_=0;pending_=active_=false;
    }
    bool selected()const{return active_;}
    bool compatible()const{return pending_;}
    int rival_wheat_flow()const{return rival_wheat_flow_;}
    uint32_t matched_days()const{return wool_.matched_days();}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a) {
        if(!active_)base_.act(o,budget,a);
        if(o.step>145 && !active_)return;
        kag::Action alternative;wool_.act(o,budget,alternative);
        if(o.step==144 && wool_.selected_route()==1) {
            const bool source=alternative.n_units==1 && alternative.units[0].op==kag::OP_WEST && alternative.n_orders==10;
            bool common=a.n_units==1 && (a.units[0].op==kag::OP_NORTH || a.units[0].op==kag::OP_WEST) && a.n_orders==7;
            for(int i=0;i<a.n_orders;++i)common&=a.orders[i].op==kag::M_HIRE;
            bool six_hires=true;for(int i=0;i<6;++i)six_hires&=alternative.orders[i].op==kag::M_HIRE;
            pending_=source && common && six_hires;
            if(pending_) {
                first_=o.shops[0];second_=o.shops[1];farmer_=a.units[0].op;old_wheat_=o.market.inventory[kag::WHEAT];
                std::copy_n(alternative.orders+6,4,deferred_.begin());
            }
        }
        if(o.step==145 && pending_) {
            int consumed=144%config_.center_sell_interval==0;
            if(144%config_.shop_sell_interval==0)for(int i=0;i<o.n_shops;++i)
                if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::WHEAT))consumed+=kag::SHOP_MULT[o.shops[i]];
            rival_wheat_flow_=o.market.inventory[kag::WHEAT]-old_wheat_+consumed;
            const bool context=v2_context(first_,second_);
            const bool broaden=mode_==2 && (first_==kag::SHOP_YARN_STORE || second_==kag::SHOP_YARN_STORE) &&
                o.opponent().n_units==7 && rival_wheat_flow_==-1;
            active_=mode_==0 || context || broaden;
            if(active_) {
                // The seventh hire moved from h1 to h0. At h1 it can reach
                // its original southeast spawn; the remaining hire then
                // receives its original northeast spawn and index.
                if(o.self().n_units!=8 || o.self().pos_x[7]!=4 || o.self().pos_y[7]!=5 || alternative.n_orders>6)std::abort();
                alternative.units[0]={uint8_t(farmer_==kag::OP_NORTH?kag::OP_WEST:kag::OP_NORTH),0,1};
                alternative.units[7]={kag::OP_EAST,0,1};
                bool removed=false;
                for(int i=0;i<alternative.n_orders;++i)if(!removed && alternative.orders[i].op==kag::M_HIRE) {
                    alternative.orders[i]={};removed=true;
                }
                if(!removed)std::abort();
                for(const auto& order:deferred_)alternative.orders[alternative.n_orders++]=order;
                alternative.n_units=o.self().n_units;alternative.finalize();
            }
        }
        if(active_)a=alternative;
    }
};
}
