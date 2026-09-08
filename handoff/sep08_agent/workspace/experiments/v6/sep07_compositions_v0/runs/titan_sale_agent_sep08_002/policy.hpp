#pragma once
#include "../empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include "../day_programs_sep08_001/model.hpp"
#include "../titan_sale_lots_sep08_001/lots.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <cstring>

namespace compositions::titan_sale_agent_v2 {
using namespace kag;
using titan_lots::Plan;
using Parent=empty_sale_slots_m2::Agent;

inline bool product(int i){return i>WHEAT && i<FERTILIZER;}
inline int offered(const Action& a,int item) {
    int n=0;for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL && a.orders[j].item==item)n+=std::max(0,int(a.orders[j].n));return n;
}
inline void solo(Sim& sim,const Action& own,int seat) {
    Action pass;pass.clear();pass.n_units=sim.st.farms[1-seat].n_units;pass.finalize();
    if(seat==0)sim.step(own,pass);else sim.step(pass,own);
}
inline Farm post_units(const Sim& before,const Action& own,int seat) {
    auto sim=before;auto units=own;units.n_orders=0;units.finalize();
    if(sim.st.hour==sim.cfg.turns_per_day-1)sim.st.hour=0;
    solo(sim,units,seat);return sim.st.farms[seat];
}
// Keep original positions, replacing withheld quantities with PASS. Add extra
// sales only at the end. Returns the actual offered quantity for this product.
inline int sell(Action& a,int item,int quantity,int capacity) {
    int left=quantity;
    for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL && a.orders[j].item==item) {
        const int n=std::min(left,std::max(0,int(a.orders[j].n)));left-=n;
        if(n)a.orders[j].n=n;else a.orders[j]={};
    }
    if(left && a.n_orders<capacity){a.orders[a.n_orders++]={M_SELL,uint8_t(item),left};left=0;}
    a.finalize();return quantity-left;
}
inline bool same_service(const Farm& a,const Farm& b,int item) {
    if(a.n_units!=b.n_units || a.n_quadrants!=b.n_quadrants || a.hires_today!=b.hires_today)return false;
    // Both forecasts begin as copies of the same reconstructed state. These
    // arrays keep identical representation unless a physical action differs.
    if(std::memcmp(a.tiles,b.tiles,sizeof a.tiles) || std::memcmp(a.seeds,b.seeds,sizeof a.seeds) ||
       std::memcmp(a.pos_x,b.pos_x,sizeof a.pos_x) || std::memcmp(a.pos_y,b.pos_y,sizeof a.pos_y) ||
       std::memcmp(a.inv,b.inv,sizeof a.inv) || std::memcmp(a.inv_keys,b.inv_keys,sizeof a.inv_keys) ||
       std::memcmp(a.inv_nkeys,b.inv_nkeys,sizeof a.inv_nkeys))return false;
    for(int i=0;i<N_ITEMS;++i) {
        if(i!=item && a.shed[i]!=b.shed[i])return false;
        if(a.discarded[i]>b.discarded[i] || a.produced[i]!=b.produced[i])return false;
    }
    return true;
}

// Horizon 0 is the exact parent control. Mode 1 uses public rival supply stress
// scenarios; mode 2 ablates that supply while retaining the same lot search.
template<int Horizon,int Mode=1> class Agent {
    Parent parent_;
    day_programs::ObservationModel model_;
    agent::AgentConfig config_;
    std::array<Plan,N_PRODUCTS> planned_{};
    std::array<std::array<int,N_PRODUCTS>,9> harvests_{};
    std::array<int,100> old_product_{},old_yield_{};
    bool previous_=false;
    uint64_t evaluated_=0,forecasts_=0;
    int chosen_=0,changed_=0,exact_rejected_=0,budget_stops_=0;
    int64_t predicted_=0;

    void observe(const agent::AgentObservation& o) {
        harvests_[o.step%9]={};
        const auto& rival=o.farms[1-o.player];
        for(int cell=0;cell<100;++cell) {
            const auto& t=rival.tiles[cell/10][cell%10];
            const int item=t.has_animal?ANIMALS[t.what-GOOSE].product:t.kind==T_PLANT?t.what:-1;
            if(previous_ && product(old_product_[cell]))
                harvests_[o.step%9][old_product_[cell]]+=std::max(0,old_yield_[cell]-int(t.yield_units));
            old_product_[cell]=item;old_yield_[cell]=std::max(0,int(t.yield_units));
        }
        previous_=true;
    }
    int rival_supply(int item)const {
        if constexpr(Mode==2)return 0;
        int visible=0,recent=0;
        for(int cell=0;cell<100;++cell)if(old_product_[cell]==item)visible+=old_yield_[cell];
        for(const auto& h:harvests_)recent+=h[item];
        return std::min(100,std::max(visible,recent));
    }
    int due(int item,int now)const {
        int n=0;for(int k=0;k<planned_[item].count;++k)if(planned_[item].sales[k].step<=now)n+=planned_[item].sales[k].quantity;return n;
    }
    void apply_due(Action& a,const Farm& post,int now)const {
        for(int item=1;item<FERTILIZER;++item)if(due(item,now))
            sell(a,item,std::min(int(post.shed[item]),offered(a,item)+due(item,now)),config_.max_orders);
    }
public:
    void reset(const agent::AgentInit& init) {
        parent_.reset(init);model_.reset(init.config);config_=init.config;planned_={};harvests_={};
        old_product_.fill(-1);old_yield_={};previous_=false;
        evaluated_=forecasts_=0;chosen_=changed_=exact_rejected_=budget_stops_=0;predicted_=0;
    }
    uint64_t evaluated()const{return evaluated_;}
    uint64_t forecasts()const{return forecasts_;}
    int chosen()const{return chosen_;}
    int changed()const{return changed_;}
    int rejected()const{return exact_rejected_;}
    int budget_stops()const{return budget_stops_;}
    int64_t predicted_gain()const{return predicted_;}

    void act(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& out) {
        parent_.act(o,budget,out);
        if constexpr(Horizon==0)return;
        observe(o);
        if(o.step<216 || o.step>=config_.episode_steps-2 || config_.shed_capacity!=100)return;
        if(budget.max_expansions==0 || budget.soft_expired() || budget.hard_expired()){planned_={};++budget_stops_;return;}
        model_.advance(o.step);const Sim start=model_.make(o);
        const Action original=out;const Farm post=post_units(start,out,o.player);
        apply_due(out,post,o.step);
        int end=std::min({o.step+Horizon,o.step+config_.turns_per_day-1-o.hour,config_.episode_steps-2});
        if constexpr(Mode>=4)for(const auto& p:planned_) {
            int last=-1;for(int k=0;k<p.count;++k)last=std::max(last,p.sales[k].step);
            if(last>o.step)end=std::min(end,last);
        }
        const int turns=end-o.step+1;
        if(turns<2){planned_={};return;}
        std::array<Action,9> route;std::array<Farm,9> expected;
        auto continuation=parent_;auto forecast=start;
        uint64_t nodes=0;
        bool stopped=false;
        auto available=[&] {
            if(stopped)return false;
            if(nodes>=budget.max_expansions || ((nodes&31)==0 && (budget.soft_expired() || budget.hard_expired()))) {
                stopped=true;++budget_stops_;return false;
            }
            ++nodes;return true;
        };
        for(int t=0;t<turns;++t) {
            if(!available()){out=original;planned_={};return;}
            if(t==0)route[t]=out;
            else {
                const auto observation=agent::runtime::make_observation(forecast,o.player);
                continuation.act(observation,budget,route[t]);
                const auto projected=post_units(forecast,route[t],o.player);
                apply_due(route[t],projected,o.step+t);
            }
            solo(forecast,route[t],o.player);expected[t]=forecast.st.farms[o.player];++forecasts_;
        }
        // Quote all requested spending with unlimited temporary cash. No sales
        // fund this reserve. This conservatively retains current sales whenever
        // available cash cannot cover the unchanged short continuation.
        auto reserve=start;reserve.cfg.shed_capacity=10000;reserve.st.farms[o.player].money=1e9;
        for(int t=0;t<turns;++t) {
            auto a=route[t];for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL)a.orders[j]={};
            a.finalize();solo(reserve,a,o.player);
        }
        const double spend=1e9-reserve.st.farms[o.player].money;
        int best_item=-1;int64_t best_gain=0;Plan best_plan;
        for(int item=1;item<FERTILIZER && !stopped;++item) {
            const int quantity=post.shed[item];if(quantity<=0)continue;
            if constexpr(Mode>=4) {
                int last=-1;for(int k=0;k<planned_[item].count;++k)last=std::max(last,planned_[item].sales[k].step);
                if(last>=0 && last<=o.step)continue; // Honor a committed final date.
            }
            titan_lots::Context c;c.item=item;c.quantity=quantity;c.inventory=o.market.inventory[item];c.now=o.step;
            c.last=config_.episode_steps-2;c.n_shops=o.n_shops;std::copy_n(o.shops,o.n_shops,c.shops.begin());
            c.shop_interval=config_.shop_sell_interval;c.center_interval=config_.center_sell_interval;
            c.rival_quantity=rival_supply(item);c.dates[c.n_dates++]=o.step;
            for(int step=o.step+1;step<end && c.n_dates<3;++step)
                if((step-1)%config_.shop_sell_interval==0 || (step-1)%config_.center_sell_interval==0)c.dates[c.n_dates++]=step;
            c.dates[c.n_dates++]=end;
            int left=quantity;
            for(int t=0;t<turns;++t) {
                const int n=std::min(left,offered(route[t],item));
                if(n){c.reference.add(o.step+t,n);left-=n;}
            }
            c.minimum_now=o.self().money<spend?std::min(quantity,offered(out,item)):0;
            // Withhold this product in a roomy forward replay, so requested
            // deposits stay visible instead of disappearing as shed overflow.
            auto roomy=start;roomy.cfg.shed_capacity=10000;roomy.st.farms[o.player].money=1e9;
            std::array<int,9> before_total{},after_total{};
            for(int t=0;t<turns;++t) {
                auto a=route[t];sell(a,item,0,config_.max_orders);
                before_total[t]=post_units(roomy,a,o.player).shed_total;
                solo(roomy,a,o.player);after_total[t]=roomy.st.farms[o.player].shed_total;
            }
            auto capacity=[&](const Plan& plan) {
                if(!available())return false;
                int sold=0;
                for(int t=0;t<turns;++t) {
                    if(t>0 && before_total[t]-sold>config_.shed_capacity-1)return false;
                    const int q=plan.at(o.step+t);
                    if(q && route[t].n_orders>=config_.max_orders && q>offered(route[t],item))return false;
                    sold+=q;if(after_total[t]-sold>config_.shed_capacity-1)return false;
                }
                if constexpr(Mode>=4) {
                    int reference_total=0;for(int k=0;k<c.reference.count;++k)reference_total+=c.reference.sales[k].quantity;
                    if(sold!=reference_total)return false;
                }
                return true;
            };
            const auto result=titan_lots::optimize(c,capacity);evaluated_+=result.plans;
            if(stopped || result.worst_gain<=best_gain)continue;
            // Spend exact scheduling effort only on a fully scored improvement.
            // Compare fixed physical actions under the same public assumptions.
            auto exact=start;auto actual_parent=parent_;bool safe=true;
            for(int t=0;t<turns && safe;++t) {
                if(!available()){safe=false;break;}
                auto a=route[t];
                if constexpr(Mode>=3)if(t>0) {
                    auto observation=agent::runtime::make_observation(exact,o.player);
                    actual_parent.act(observation,budget,a);
                    const auto projected=post_units(exact,a,o.player);apply_due(a,projected,o.step+t);
                    if(a.n_units!=route[t].n_units){safe=false;break;}
                    for(int u=0;u<a.n_units;++u)if(a.units[u].op!=route[t].units[u].op || a.units[u].arg!=route[t].units[u].arg || a.units[u].n!=route[t].units[u].n)safe=false;
                    if(!safe)break;
                }
                const int q=result.plan.at(o.step+t);
                if(sell(a,item,q,config_.max_orders)!=q){safe=false;break;}
                solo(exact,a,o.player);
                safe=same_service(exact.st.farms[o.player],expected[t],item);
            }
            if constexpr(Mode>=4)safe=safe && exact.st.farms[o.player].shed[item]==expected[turns-1].shed[item];
            if(!safe){++exact_rejected_;continue;}
            best_item=item;best_gain=result.worst_gain;best_plan=result.plan;
        }
        if(best_item>=0) {
            sell(out,best_item,best_plan.at(o.step),config_.max_orders);
            planned_[best_item]=best_plan;++chosen_;predicted_+=best_gain;
        }
        for(int item=1;item<FERTILIZER;++item) {
            Plan future;int remaining=std::max(0,int(post.shed[item])-offered(out,item));
            for(int k=0;k<planned_[item].count;++k) {
                const auto part=planned_[item].sales[k];
                if(part.step>o.step && remaining>0) {
                    const int q=std::min(remaining,part.quantity);future.add(part.step,q);remaining-=q;
                }
            }
            planned_[item]=future;
        }
        bool changed=out.n_orders!=original.n_orders;
        for(int j=0;j<std::min(out.n_orders,original.n_orders);++j)
            changed|=out.orders[j].op!=original.orders[j].op || out.orders[j].item!=original.orders[j].item || out.orders[j].n!=original.orders[j].n;
        changed_+=changed;out.finalize();
    }
};
}
