#pragma once
#include "biology.hpp"
#include "composition.hpp"
#include "economics.hpp"
#include <bit>
#include <cmath>
#include <numeric>
#include <vector>

namespace compositions {
enum class ServiceModel {Productive, Fertilized, Recorded};
struct EstimateOptions {
    ServiceModel service=ServiceModel::Productive;
    bool recorded_layout=true,recorded_support=false;
    double travel_per_operation=1.0;
    int deposit_bundle=12,reserve_days=1,max_hands=20;
};

struct EstimatedPlan {
    FinancialPlan financial;
    ProductVector produced{};
    std::array<int,30> field_operations{},work_turns{},hands{},quadrants{},occupancy{};
    int input_wheat=0,input_fertilizer=0,tile_conflicts=0,unplaced_lives=0;
    int uncovered_work=0,seed_cost=0,animal_cost=0,hire_cost=0,land_cost=0;
    int first_layout_conflict=-1;
};

inline int shed_distance(int x,int y) {
    return std::min(std::abs(x-4),std::abs(x-5))+std::min(std::abs(y-4),std::abs(y-5));
}

// A heuristic resource schedule, not a worker solver or certified bound.
// The same typed dated composition can use observed or newly assigned tiles,
// recorded or productive service, and recorded or estimated support.
inline EstimatedPlan estimate_plan(std::span<const Life> proposal,const Support& support,const EstimateOptions& options) {
    using namespace kag;
    if(options.deposit_bundle<1 || options.max_hands<0 || options.max_hands>20)std::abort();
    EstimatedPlan result;result.financial.reserve_days=options.reserve_days;
    std::vector<Life> lives(proposal.begin(),proposal.end());
    std::stable_sort(lives.begin(),lives.end(),[](const Life& a,const Life& b){return a.start<b.start;});
    std::array<int,100> release{},previous{};previous.fill(-1);
    std::array<double,30> distance_work{};
    std::array<int,30> farthest{},new_land{};
    for(auto& life:lives) {
        if(life.start<0 || life.start>=life.end || life.end>719 || life.x<0 || life.x>=10 || life.y<0 || life.y>=10)std::abort();
        if(!options.recorded_layout) {
            int best=-1;double score=1e100;
            for(int cell=0;cell<100;++cell)if(release[cell]<=life.start) {
                int x=cell%10,y=cell/10;
                double value=5*quadrant_of(x,y,10)+shed_distance(x,y)*(is_animal(life.item)?2.0:1.0)
                    -(is_animal(life.item)&&is_animal(previous[cell])?1.5:0.0)+cell*0.0001;
                if(value<score) {score=value;best=cell;}
            }
            if(best<0) {++result.unplaced_lives;continue;}
            life.x=best%10;life.y=best/10;
        }
        const int cell=life.y*10+life.x,dist=shed_distance(life.x,life.y);
        if(release[cell]>life.start) {
            ++result.tile_conflicts;
            if(result.first_layout_conflict<0)result.first_layout_conflict=life.start;
        }
        release[cell]=std::max(release[cell],life.end);previous[cell]=life.item;
        Cohort cohort{uint8_t(life.item),1,life.start/24,std::min(30,(life.end+23)/24)};
        Service service=productive_service(cohort,options.service==ServiceModel::Fertilized);
        if(options.service==ServiceModel::Recorded)
            service={life.water,life.feed,life.care,life.collect,life.harvest,life.fertilize};
        auto biological=biology(cohort,service);
        result.seed_cost+=biological.seed_cost;result.animal_cost+=biological.animal_cost;
        // Purchase before the approximate outward trip. Cash gaps remain
        // visible rather than silently delaying the requested composition.
        int buy_step=std::max(0,life.start-dist-2);
        result.financial.fixed_cost[buy_step]+=biological.seed_cost+biological.animal_cost;
        // Orders for same-item purchases are merged below by a separate mask.
        new_land[cohort.start_day]=std::max(new_land[cohort.start_day],quadrant_of(life.x,life.y,10)+1);
        for(int day=0;day<30;++day) {
            const auto& b=biological.days[day];
            result.occupancy[day]+=b.active;result.field_operations[day]+=b.operations;
            result.input_wheat+=b.wheat;result.input_fertilizer+=b.fertilizer;
            if(b.operations)farthest[day]=std::max(farthest[day],dist);
            const int available=day==cohort.start_day?life.start%24:0;
            // Inputs are withdrawn before service; new purchases cannot supply
            // the current turn. Real route-derived hours replace these later.
            const int use_hour=std::clamp(std::max(1,available-dist),1,22);
            const int use_step=std::min(718,day*24+use_hour);
            result.financial.use[use_step][WHEAT]+=b.wheat;
            result.financial.use[use_step][FERTILIZER]+=b.fertilizer;
            const int output=std::accumulate(std::begin(b.output),std::end(b.output),0);
            distance_work[day]+=options.travel_per_operation*b.operations;
            distance_work[day]+=(output+b.wheat+b.fertilizer)*double(2*dist+1)/options.deposit_bundle;
            const int hour=std::clamp(std::max(available+2,2*dist+2+cell%4),1,22);
            for(int item=0;item<N_PRODUCTS;++item) {
                result.produced[item]+=b.output[item];
                result.financial.arrivals[day*24+hour][item]+=b.output[item];
            }
        }
    }
    // Count each crop/animal purchase type at each predicted purchase turn once.
    std::array<uint16_t,turns> purchase_items{};
    for(const auto& life:lives) {
        int step=std::max(0,life.start-shed_distance(life.x,life.y)-2);
        purchase_items[step]|=uint16_t{1}<<life.item;
    }
    int land=1;
    for(int day=0;day<30;++day) {
        int step=day*24;
        result.work_turns[day]=result.field_operations[day]+int(std::ceil(distance_work[day]))+2*farthest[day];
        int needed=std::max(0,(result.work_turns[day]+21)/22-1);
        int hands=options.recorded_support?support.hands[day]:std::min(options.max_hands,needed);
        result.hands[day]=hands;
        result.uncovered_work+=std::max(0,result.work_turns[day]-22*(1+hands));
        int cost=0;for(int i=0;i<hands;++i)cost+=fib(i);
        result.hire_cost+=cost;result.financial.fixed_cost[step]+=cost;
        result.financial.fixed_orders[step]+=hands;
        int target=options.recorded_support?support.quadrants[day]:std::max(land,new_land[day]);
        if(target<land || target>4)std::abort();
        for(int i=land;i<target;++i) {
            result.land_cost+=LAND_PRICES[i-1];result.financial.fixed_cost[step]+=LAND_PRICES[i-1];
            ++result.financial.fixed_orders[step];
        }
        land=target;result.quadrants[day]=land;
    }
    for(int step=0;step<turns;++step)result.financial.fixed_orders[step]+=std::popcount(purchase_items[step]);
    return result;
}
}
