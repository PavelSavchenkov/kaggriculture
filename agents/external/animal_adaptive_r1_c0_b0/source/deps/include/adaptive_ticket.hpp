#pragma once
#include "animal_ticket.hpp"
#include "herd_forecast.hpp"
#include "terminal_layer.hpp"

namespace catalog_animal_adaptive_r1_c0_b0_compositions {
struct AdaptiveAnimalPlan {
    AnimalEdit edit;
    int start_day=0,end_day=30;
    Service service;
};

class AdaptiveTicketAgent {
    using Source=TerminalAgent<top_replay_library::Agent,3>;
    AnimalTicketOverlay<Source> agent_;
    AdaptiveAnimalPlan plan_;
    std::array<Biology,3> biology_;
    std::array<double,3> scores_{};
    HerdOutput own_plan_{};
    int decision_step_,chosen_;
    double buffer_;
    int mode_;
public:
    // Modes:0 projected cash,1 projected margin,2 observed shop counts,
    // 3 observed product demand per shop cycle. Modes4/5 delay the cow/sheep
    // projection to purchase;6/7 also use the known own dated output plan.
    AdaptiveTicketAgent(int program,AdaptiveAnimalPlan plan,double buffer,int mode,bool change=true,HerdOutput own_plan={})
        :agent_(Source(top_replay_library::Agent(program)),plan.edit,change,true),plan_(plan),
         own_plan_(own_plan),
         decision_step_(mode>=2?plan.edit.purchase.step:std::min(plan.edit.purchase.step,plan.edit.structure.step)),
         chosen_(plan.edit.original),buffer_(buffer),mode_(mode) {
        if(plan.edit.structure.step<0 || mode<0 || mode>7 || (mode>=2 && plan.edit.original==kag::GOOSE))std::abort();
        for(int item=kag::GOOSE;item<=kag::SHEEP;++item)
            biology_[item-kag::GOOSE]=biology({uint8_t(item),1,plan.start_day,plan.end_day},plan.service);
    }
    static kag::agent::AgentInfo info(){return {"adaptive_ticket"};}
    void reset(const kag::agent::AgentInit& init) {
        agent_.reset(init);chosen_=plan_.edit.original;agent_.set_replacement(chosen_);scores_.fill(0);
    }
    int chosen() const {return chosen_;}
    const auto& scores() const {return scores_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        if(o.step==decision_step_) {
            if(mode_==2 || mode_==3) {
                scores_.fill(0);scores_[0]=-1e100;
                for(int i=0;i<o.n_shops;++i) {
                    const int n=mode_==2?1:kag::SHOP_MULT[o.shops[i]];
                    if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::MILK))scores_[kag::COW-kag::GOOSE]+=n;
                    if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::WOOL))scores_[kag::SHEEP-kag::GOOSE]+=n;
                }
            } else {
                auto herds=forecast_herds(o);
                if(mode_>=6) {
                    herds[0]=own_plan_;
                    for(int day=0;day<30;++day)for(int product=kag::EGG;product<=kag::WOOL;++product)
                        herds[0][day][product-kag::EGG]-=biology_[plan_.edit.original-kag::GOOSE].days[day].output[product];
                }
                for(int item=kag::GOOSE;item<=kag::SHEEP;++item) {
                    if(mode_>=4 && item==kag::GOOSE){scores_[0]=-1e100;continue;}
                    const auto value=value_herd_choice(o,herds,biology_[item-kag::GOOSE]);
                    scores_[item-kag::GOOSE]=(mode_&1)?value.margin():value.own;
                }
            }
            const double baseline=scores_[plan_.edit.original-kag::GOOSE];
            for(int item=kag::GOOSE;item<=kag::SHEEP;++item)
                if(scores_[item-kag::GOOSE]>baseline+buffer_ && scores_[item-kag::GOOSE]>scores_[chosen_-kag::GOOSE])chosen_=item;
            agent_.set_replacement(chosen_);
        }
        agent_.act(o,b,a);
    }
};
}
