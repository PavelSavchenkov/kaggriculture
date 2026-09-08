#pragma once
#include "../../rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"

namespace catalog_wool_contract_repair_v2_compositions::wool_contract_repair {
// The donor buys two sheep at217. One is picked up the following day at253.
// Retry an observed failed purchase only before that existing pickup deadline.
class Policy {
    kag::catalog_wool_contract_repair_v2_agents::rival_wool_context_v3::Agent base_;
    bool active_=false;
    int capacity_=100;
    int expected_=0,debt_=0,retry_count_=0,retry_before_=0;
    static bool same_action(const kag::Action& a,const kag::Action& b) {
        if(a.n_units!=b.n_units || a.n_orders!=b.n_orders)return false;
        for(int i=0;i<a.n_units;++i)if(a.units[i].op!=b.units[i].op || a.units[i].arg!=b.units[i].arg || a.units[i].n!=b.units[i].n)return false;
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op!=b.orders[i].op || a.orders[i].item!=b.orders[i].item || a.orders[i].n!=b.orders[i].n)return false;
        return true;
    }
    static int sheep_count(const kag::agent::AgentObservation& o) {
        int count=o.own.shed[kag::SHEEP];
        for(int u=0;u<o.self().n_units;++u)count+=o.own.inv[u][kag::SHEEP];
        for(int y=0;y<10;++y)for(int x=0;x<10;++x) {
            const auto& tile=o.self().tiles[y][x];
            count+=tile.has_animal && tile.what==kag::SHEEP;
        }
        return count;
    }
public:
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);active_=false;capacity_=init.config.shed_capacity;expected_=debt_=retry_count_=retry_before_=0;
    }
    static kag::agent::AgentInfo info(){return {"wool_contract_repair_v2"};}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a) {
        base_.act(o,budget,a);
        if(o.step==216) {
            active_=base_.selected();
            for(const auto& g:v52_family::improved_days())if(g.plan.day==9 && g.matches(o) && same_action(a,g.plan.actions[0]))active_=true;
        }
        if(!active_)return;
        if(o.step==263 && a.n_orders==0) {
            bool stable_wheat=true;
            for(int u=0;u<a.n_units;++u)stable_wheat &= a.units[u].op!=kag::OP_FEED && a.units[u].op!=kag::OP_HARVEST;
            int wheat=o.own.shed[kag::WHEAT],total=o.own.shed_total;
            for(int u=0;u<o.self().n_units;++u)for(int item=0;item<kag::N_ITEMS;++item) {
                total+=o.own.inv[u][item];if(item==kag::WHEAT)wheat+=o.own.inv[u][item];
            }
            for(const auto& g:v52_family::improved_days())if(g.plan.day==11 && stable_wheat && total<capacity_ &&
                    wheat==g.shed[kag::WHEAT]-1 && o.self().money>=kag::market_price(kag::WHEAT,o.market.inventory[kag::WHEAT]-1)) {
                a.orders[a.n_orders++]={kag::M_BUY_PRODUCT,kag::WHEAT,1};a.finalize();break;
            }
            return;
        }
        if(o.step<217 || o.step>253)return;
        if(o.step==217) {
            int requested=0;
            for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_BUY_ANIMAL && a.orders[i].item==kag::SHEEP)
                requested+=std::max(1,int(a.orders[i].n));
            if(requested==2)expected_=sheep_count(o)+requested;
            return;
        }
        if(o.step==218 && expected_)debt_=std::max(0,expected_-sheep_count(o));
        if(retry_count_) {
            debt_-=std::clamp(sheep_count(o)-retry_before_,0,retry_count_);
            retry_count_=0;
        }
        if(o.step==253 || !debt_ || o.hour==23 || a.n_orders>=10)return;
        // Let original sheep purchases run alone; count only this retry's
        // purchases on the next ordinary (non-day-end) transition.
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_BUY_ANIMAL && a.orders[i].item==kag::SHEEP)return;
        const int count=std::min({debt_,int(o.self().money)/500,capacity_-int(o.own.shed_total)});
        if(count<=0)return;
        retry_count_=count;retry_before_=sheep_count(o);
        a.orders[a.n_orders++]={kag::M_BUY_ANIMAL,kag::SHEEP,count};a.finalize();
    }
};
}
