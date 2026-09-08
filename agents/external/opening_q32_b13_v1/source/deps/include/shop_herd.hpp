#pragma once
#include "adaptive_ticket.hpp"
#include "../runs/justin_day_library_001/days.hpp"

namespace catalog_opening_q32_b13_v1_compositions {
// User's observed milk/wool shop rule, applied to several independent purchases.
// Choose at purchase-day start: shops cannot change again before that purchase.
// A changed purchase/pickup/place day uses its original addressed course. Other
// compiled days require their complete original physical starting contract.
class ShopHerdAgent {
    TerminalAgent<top_replay_library::Agent,3> source_{top_replay_library::Agent(150)};
    std::vector<AdaptiveAnimalPlan> plans_;
    std::vector<AnimalTicketPatch> patches_;
    std::vector<int> chosen_;
    std::vector<GuardedDay> days_;
    std::array<int,30> day_index_;
    int mode_;
    bool use_day_=false;
    uint32_t matched_days_=0;
public:
    ShopHerdAgent(std::vector<AdaptiveAnimalPlan> plans,int mode,bool use_days)
        :plans_(std::move(plans)),mode_(mode) {
        if(mode!=2 && mode!=3)std::abort();
        for(const auto& p:plans_) {
            if(p.edit.original!=kag::COW && p.edit.original!=kag::SHEEP)std::abort();
            patches_.emplace_back(p.edit);chosen_.push_back(p.edit.original);
        }
        day_index_.fill(-1);
        if(use_days)for(const auto& entry:day_library::entries()) {
            day_index_[entry.day.plan.day]=days_.size();days_.push_back(entry.day);
        }
    }
    static kag::agent::AgentInfo info(){return {"shop_herd"};}
    void reset(const kag::agent::AgentInit& init) {
        source_.reset(init);use_day_=false;matched_days_=0;
        for(int i=0;i<int(plans_.size());++i) {
            patches_[i].reset();chosen_[i]=plans_[i].edit.original;patches_[i].set_replacement(chosen_[i]);
        }
    }
    const auto& chosen() const {return chosen_;}
    uint32_t matched_days() const {return matched_days_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        if(o.hour==0) {
            int milk=0,wool=0;
            for(int i=0;i<o.n_shops;++i) {
                const int n=mode_==2?1:kag::SHOP_MULT[o.shops[i]],mask=kag::SHOP_MASK[o.shops[i]];
                if(mask&(1u<<kag::MILK))milk+=n;
                if(mask&(1u<<kag::WOOL))wool+=n;
            }
            for(int i=0;i<int(plans_.size());++i)if(o.day==plans_[i].edit.purchase.step/24) {
                chosen_[i]=milk>wool?kag::COW:wool>milk?kag::SHEEP:plans_[i].edit.original;
                patches_[i].set_replacement(chosen_[i]);
            }
            use_day_=day_index_[o.day]>=0 && days_[day_index_[o.day]].matches(o);
            for(int i=0;use_day_ && i<int(plans_.size());++i)if(chosen_[i]!=plans_[i].edit.original)
                for(auto address:{plans_[i].edit.purchase,plans_[i].edit.pickup,plans_[i].edit.placement})
                    if(address.step/24==o.day)use_day_=false;
        }
        source_.act(o,b,a);
        if(use_day_) {
            matched_days_|=uint32_t(1)<<o.day;a=days_[day_index_[o.day]].plan.actions[o.hour];
            for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};
            a.n_units=o.self().n_units;
        }
        for(int i=0;i<int(plans_.size());++i)if(chosen_[i]!=plans_[i].edit.original)patches_[i].amend(o,a);
        a.finalize();
    }
};
}
