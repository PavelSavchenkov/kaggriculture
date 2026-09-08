#pragma once
#include "data.hpp"
#include "../../opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp"

namespace catalog_animal_repair_q24_premium_m2_sale::late_portfolio {
struct Prediction {
    std::array<double,4> own{},margin{},deviation{},positive_fraction{};
    std::array<bool,4> eligible{};
    double seconds=0;
};
class Policy {
    catalog_animal_repair_q24_premium_m2_sale::opening_q32_b13_v1::Agent base_;
    int samples_,forced_,choice_=0,selected_day_=-1;
    double threshold_,risk_,own_weight_;
    bool wheat_context_=false,berry_=false;
    uint32_t matched_=0;
    Prediction prediction_;
public:
    Policy(int samples=32,double threshold=0,double risk=0,double own_weight=0,int forced=-1):
        samples_(samples),forced_(forced),threshold_(threshold),risk_(risk),own_weight_(own_weight) {
        if(samples<8 || samples>64 || samples%8 || forced<-1 || forced>3)std::abort();
        (void)calendars();
    }
    static kag::agent::AgentInfo info(){return {"late_portfolio"};}
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);choice_=0;selected_day_=-1;wheat_context_=berry_=false;matched_=0;prediction_={};
    }
    const Prediction& prediction()const{return prediction_;}
    int choice()const{return choice_;}
    uint32_t matched_days()const{return matched_;}
    void evaluate(const kag::agent::AgentObservation& o) {
        const auto begin=std::chrono::steady_clock::now();
        const auto& courses=calendars();const Biology empty;
        std::array<double,4> squares{};
        prediction_.eligible[0]=true;
        for(int c=1;c<4;++c)prediction_.eligible[c]=wheat_context_ && courses[c].days[0][0].matches(o);
        if(wheat_context_)for(int sample=0;sample<samples_;++sample) {
            const auto future=scenario(o,sample);const int leaf=future.berry;
            const auto baseline=value_animal_investment(o,courses[0].flows[leaf],empty,0,1,&future.demand);
            for(int c=1;c<4;++c)if(prediction_.eligible[c]) {
                const auto value=value_animal_investment(o,courses[c].flows[leaf],empty,0,1,&future.demand);
                const double fixed=courses[c].fixed_cost[leaf]-courses[0].fixed_cost[leaf];
                const double own=value.own-baseline.own-fixed,margin=value.margin()-baseline.margin()-fixed;
                prediction_.own[c]+=own;prediction_.margin[c]+=margin;squares[c]+=margin*margin;
                prediction_.positive_fraction[c]+=margin>0;
            }
        }
        double best=threshold_;
        for(int c=1;c<4;++c)if(prediction_.eligible[c]) {
            prediction_.own[c]/=samples_;prediction_.margin[c]/=samples_;prediction_.positive_fraction[c]/=samples_;
            prediction_.deviation[c]=std::sqrt(std::max(0.,squares[c]/samples_-prediction_.margin[c]*prediction_.margin[c]));
            const double value=(1-own_weight_)*prediction_.margin[c]+own_weight_*prediction_.own[c]-risk_*prediction_.deviation[c];
            if(value>best){best=value;choice_=c;}
        }
        if(forced_>=0)choice_=prediction_.eligible[forced_]?forced_:0;
        prediction_.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.hour==0) {
            if(o.day==12) {
                int demand=0;for(int i=0;i<o.n_shops;++i)demand+=bool(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::TOMATO));
                wheat_context_=demand<2;
            }
            if(o.day==13)evaluate(o);
            if(o.day==20) {
                int demand=0;for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[i]];
                berry_=demand>=4;
            }
            selected_day_=-1;
            if(choice_ && o.day>=13 && o.day<30) {
                const int index=o.day-13;const auto& day=calendars()[choice_].days[berry_][index];
                if(day.matches(o))selected_day_=index;
            }
        }
        if(selected_day_<0)return;
        matched_|=uint32_t(1)<<o.day;
        action=calendars()[choice_].days[berry_][selected_day_].plan.actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
}
