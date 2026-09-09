#pragma once
#include "../../rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"

namespace compositions::rival_wool_repair {
// The donor buys two sheep at217. One is picked up the following day at253.
// Retry an observed failed purchase only before that existing pickup deadline.
class Policy {
    kag::agents::rival_wool_context_v3::Agent base_;
    int expected_=0,debt_=0,retry_count_=0,retry_before_=0;
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
        base_.reset(init);expected_=debt_=retry_count_=retry_before_=0;
    }
    static kag::agent::AgentInfo info(){return {"rival_wool_purchase_repair_v1"};}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a) {
        base_.act(o,budget,a);
        if(!base_.selected() || o.step<217 || o.step>253)return;
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
        const int count=std::min({debt_,int(o.self().money)/500,100-int(o.own.shed_total)});
        if(count<=0)return;
        retry_count_=count;retry_before_=sheep_count(o);
        a.orders[a.n_orders++]={kag::M_BUY_ANIMAL,kag::SHEEP,count};a.finalize();
    }
};
}
