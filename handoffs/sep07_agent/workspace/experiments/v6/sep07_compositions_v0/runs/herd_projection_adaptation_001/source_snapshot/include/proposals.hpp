#pragma once
#include "estimate.hpp"
#include <string>

namespace compositions {
struct Proposal {
    std::vector<Life> lives;
    Support support;
    int id=0,parent=-1,source_program=-1;
    std::string edit="source";
    bool recorded_layout=true,recorded_support=true,recorded_service=true;
    double estimate=-1e100,estimated_cash=0,estimated_margin=0,min_cash=0;
    double exact_utility=-1,exact_margin=-1e100,exact_cash=0;
    int layout_failures=0,work_gap=0;
    uint64_t phenotype=0;
};

inline bool overlaps(const Life& a,const Life& b) {return a.start<b.end && b.start<a.end;}

inline bool place_life(Proposal& proposal,Life life) {
    int best=-1;double score=1e100;
    for(int cell=0;cell<100;++cell) {
        int x=cell%10,y=cell/10;bool occupied=false;
        for(const auto& old:proposal.lives)
            if(old.x==x && old.y==y && overlaps(life,old)) {occupied=true;break;}
        if(occupied)continue;
        const double cost=5*kag::quadrant_of(x,y,10)+shed_distance(x,y)+(kag::is_animal(life.item)?shed_distance(x,y):0)+cell*0.0001;
        if(cost<score) {score=cost;best=cell;}
    }
    if(best<0)return false;
    life.x=best%10;life.y=best/10;proposal.lives.push_back(life);return true;
}

inline Life productive_life(int item,int start_day,int end_day,bool fertilize) {
    Cohort cohort{uint8_t(item),1,start_day,end_day};
    const auto service=productive_service(cohort,fertilize);
    return {item,start_day*24,std::min(719,end_day*24),0,0,service.fertilize,service.water,
        service.feed,service.care,service.collect_fertilizer,service.harvest};
}

inline void close_land_dependencies(Proposal& proposal) {
    if(!proposal.recorded_support)return;
    int land=1;
    for(int day=0;day<30;++day) {
        land=std::max(land,proposal.support.quadrants[day]);
        for(const auto& life:proposal.lives)if(life.start/24<=day)
            land=std::max(land,kag::quadrant_of(life.x,life.y,10)+1);
        proposal.support.quadrants[day]=land;
    }
}

inline uint32_t shift_service(uint32_t mask,int delta) {
    return delta>0?(mask<<delta)&season_days:mask>>(-delta);
}

template<class Random>
bool mutate(Proposal& p,Random random) {
    if(p.lives.empty())return false;
    const int operation=random()%5,index=random()%p.lives.size();
    if(operation==0) {
        auto life=p.lives[index];
        if(!place_life(p,life))return false;
        p.edit="increase_dated_count";
    } else if(operation==1) {
        p.lives.erase(p.lives.begin()+index);p.edit="decrease_dated_count";
    } else if(operation==2) {
        const int day_delta=random()%2?1:-1;
        auto life=p.lives[index];
        if(life.start+day_delta*24<0 || life.start+day_delta*24>=718)return false;
        life.start+=day_delta*24;life.end=std::clamp(life.end+day_delta*24,life.start+1,719);
        for(auto mask:{&life.fertilize,&life.water,&life.feed,&life.care,&life.collect,&life.harvest})*mask=shift_service(*mask,day_delta);
        p.lives.erase(p.lives.begin()+index);
        if(!place_life(p,life))return false;
        p.edit="shift_lifecycle_one_day";
    } else if(operation==3) {
        int item=p.lives[index].item;
        std::erase_if(p.lives,[&](const Life& life){return life.item==item;});
        p.edit="remove_product_family";
    } else {
        constexpr int products[]={0,1,2,3,4,9,10,11};
        int item=products[random()%8],day=3+random()%5;
        int end=kag::is_animal(item)?30:std::min(30,day+(kag::CROPS[item].ongoing?
            kag::CROPS[item].first_yield_day+(kag::CROPS[item].max_yield-1)*kag::CROPS[item].interval+1:
            kag::CROPS[item].max_yield_day+1));
        auto life=productive_life(item,day,end,item==kag::TOMATO || item==kag::STRAWBERRY);
        for(int n=0;n<3;++n)if(!place_life(p,life))return false;
        p.edit="insert_three_lifecycles";
    }
    close_land_dependencies(p);
    p.exact_utility=-1;p.exact_margin=-1e100;p.phenotype=0;
    return !p.lives.empty();
}

template<class Random>
Proposal cold_proposal(Random random) {
    Proposal p;p.recorded_support=false;p.recorded_layout=true;p.recorded_service=false;p.edit="cold_rule_build";
    int cows=random()%5,sheep=random()%5,geese=random()%7,wheat=random()%17,melon=random()%13,strawberry=random()%13;
    auto add=[&](int item,int count,int day,int end) {
        for(int i=0;i<count;++i)place_life(p,productive_life(item,day,end,item==kag::STRAWBERRY));
    };
    add(kag::COW,cows,0,30);add(kag::SHEEP,sheep,0,30);add(kag::GOOSE,geese,3,30);
    add(kag::MELON,melon,0,13);add(kag::STRAWBERRY,strawberry,5,22);
    for(int day=0;day<=24;day+=5)add(kag::WHEAT,wheat,day,day+5);
    return p;
}

inline void estimate_proposal(Proposal& proposal,std::span<const EconomicScenario> scenarios) {
    EstimateOptions options;
    options.recorded_layout=proposal.recorded_layout;options.recorded_support=proposal.recorded_support;
    options.service=proposal.recorded_service?ServiceModel::Recorded:ServiceModel::Productive;
    auto plan=estimate_plan(proposal.lives,proposal.support,options);
    proposal.layout_failures=plan.tile_conflicts+plan.unplaced_lives;proposal.work_gap=plan.uncovered_work;
    double score=0,cash=0,margin=0,minimum=0;
    for(const auto& scenario:scenarios) {
        auto value=economics(plan.financial,scenario);
        cash+=value.cash;margin+=value.cash-value.rival_cash;minimum+=value.min_cash;
        // Funding is an uncertain schedule gap, so it is a soft penalty. This
        // proposal score is never used as an exact promotion result.
        score+=value.cash-value.rival_cash-0.25*std::max(0.0,-value.min_cash);
    }
    proposal.estimated_cash=cash/scenarios.size();proposal.estimated_margin=margin/scenarios.size();proposal.min_cash=minimum/scenarios.size();
    proposal.estimate=proposal.layout_failures?-1e100:score/scenarios.size();
}
}
