#include "workload.hpp"
#include <algorithm>
#include <climits>
#include <tuple>

namespace kag::day_compiler {
Workload::Workload() = default;
int defer_newborn_service(const Observation& o,const detail::BoundIntent& intent,Workload& work) {
    int removed=0;
    auto defer=[&](auto& events,int animal) {
        if(!(events&worker::Feed))return;
        events&=~(worker::Feed|worker::Care); ++removed;
        ++work.newborn_service_deferred[animal-GOOSE];
        work.newborn_first_yield_shortfall+=animal!=COW && o.day+ANIMALS[animal-GOOSE].first_yield_day<=29;
    };
    for(int c=0;c<100;++c) {
        const auto& tile=work.input.grid[c];
        if(tile.has_animal && tile.planted_day==0 && !intent.serve[c] && !intent.escape[c])defer(work.input.events[c],tile.what);
    }
    for(int j=0;j<work.input.establish_count;++j) {
        auto& newborn=work.input.establish[j]; if(is_animal(newborn.product))defer(newborn.events,newborn.product);
    }
    if(!removed)return 0;
    int feeds=0,carried=0;
    for(int e:work.input.events)feeds+=bool(e&worker::Feed);
    for(int j=0;j<work.input.establish_count;++j)feeds+=bool(work.input.establish[j].events&worker::Feed);
    for(int u=0;u<work.input.worker_count;++u)carried+=work.input.workers[u].inventory[WHEAT];
    work.field_wheat_used=std::min(carried,std::max(0,feeds-work.input.shed[WHEAT]));
    work.wheat_need=std::max(0,feeds-work.input.shed[WHEAT]-carried);
    return removed;
}
namespace {
int select(const Calendars& plans, CalendarChoice choice) {
    auto key=[&](const Calendar& c) {
        const int today=(c.water_days&1)+(c.fertilizer_days&1);
        const int actions=c.waters+c.fertilizers;
        if(choice==CalendarChoice::FewerInputs) return std::tuple{int(c.fertilizers),actions,today};
        if(choice==CalendarChoice::FewerActions) return std::tuple{actions,int(c.fertilizers),today};
        return std::tuple{today,int(c.fertilizers),actions};
    };
    int best=0;
    for(int i=1;i<plans.count;++i) if(key(plans.plans[i])<key(plans.plans[best])) best=i;
    return best;
}
bool produces_tonight(const Tile& tile,int day) {
    if(day==29) return false;
    if(tile.has_animal) {
        const auto& a=ANIMALS[tile.what-GOOSE]; const int age=day+1-tile.planted_day;
        return age>=a.first_yield_day && (age-a.first_yield_day)%a.interval==0;
    }
    const auto& c=CROPS[tile.what]; const int age=day+1-tile.planted_day;
    return c.ongoing && age>=c.first_yield_day && (age-c.first_yield_day)%c.interval==0 &&
           age<=c.first_yield_day+c.interval*(c.max_yield-1);
}
}
int planned_harvest_yield(const Tile& tile,int day,int events) {
    if(!(events&worker::Harvest)) return 0;
    int yield=tile.yield_units;
    if(tile.kind==T_PLANT && !CROPS[tile.what].ongoing && (events&worker::Water)) {
        const auto& crop=CROPS[tile.what]; const int age=day-tile.planted_day;
        if(age>=(crop.max_yield_day+1)/2 && age<=crop.max_yield_day)
            yield=std::min(crop.max_yield,yield+1+int(tile.fertilized_until_day>=day || (events&worker::Fertilize)));
    }
    return yield;
}
bool WorkloadBuilder::build(const Observation& dawn,const detail::BoundIntent& intent,Workload& out,
                            CalendarChoice choice,bool optional_harvests,bool direct_field_inputs,bool optional_collections,bool productive_newborns) {
    out=Workload(); auto& input=out.input;
    input.hours=std::min(24,720-dawn.step-1);
    std::copy_n(dawn.own.shed,N_ITEMS,input.shed); std::copy_n(dawn.own.seeds,N_CROPS,input.seeds);
    int retained[3]{}, feeds=0, fertilizers=0, collections=0;
    for(int cell=0;cell<100;++cell) {
        const auto& tile=dawn.self().tiles[cell/10][cell%10]; auto& normalized=input.grid[cell];
        normalized=tile; normalized.planted_day-=dawn.day; normalized.fertilized_until_day-=dawn.day;
        normalized.max_lifespan_step=tile.max_lifespan_step<0?INT_MAX:tile.max_lifespan_step-dawn.step;
        int events=0;
        if(tile.has_animal) {
            retained[tile.what-GOOSE]+=!intent.escape[cell];
            if(intent.serve[cell]) {
                events|=worker::Feed; ++feeds;
                if(!zero_value_care(tile,dawn.day)) events|=worker::Care;
            }
            if(tile.yield_units && optional_harvests) events|=worker::Harvest;
            if(tile.fertilizer_available && optional_harvests && optional_collections) { events|=worker::CollectFertilizer; ++collections; }
        } else if(tile.kind==T_PLANT) {
            const auto& crop=CROPS[tile.what]; const auto goal=intent.crops[cell];
            // Optional liquidation must not turn uncollectable dawn stock into
            // a mandatory receipt. Build the incumbent from intact reachable
            // yields; partially decayed optional harvests are separate candidates.
            const int expiry=tile.max_lifespan_step<0?INT_MAX:tile.max_lifespan_step-dawn.step;
            const int first_loss=expiry<0?(-expiry&1):expiry;
            const int farmer=dawn.self().pos_y[0]*BOARD+dawn.self().pos_x[0];
            const int earliest=std::min(distance(farmer,cell),1+shed_distance(cell));
            const bool intact_reachable=earliest<=first_loss;
            if(goal.mode!=CropMode::Retire) {
                const auto plans=crop.ongoing?biology_.ongoing(tile,dawn.day,goal.mode==CropMode::Full):
                    biology_.one_shot(tile,dawn.day,goal.harvest_age,goal.min_yield);
                if(!plans.count) return false;
                const auto calendar=plans.plans[select(plans,choice)];
                if(calendar.water_days&1) events|=worker::Water;
                if(calendar.fertilizer_days&1) { events|=worker::Fertilize; ++fertilizers; }
                if(!crop.ongoing && dawn.day-tile.planted_day>=goal.harvest_age) events|=worker::Harvest;
                if(crop.ongoing && produces_tonight(tile,dawn.day) &&
                    tile.yield_units+(goal.mode==CropMode::Full?2:1)>crop.max_yield) events|=worker::Harvest;
            }
            const bool ripe=dawn.day-tile.planted_day>=crop.first_yield_day;
            if(tile.yield_units && ripe && intact_reachable && optional_harvests && (crop.ongoing || goal.mode==CropMode::Retire)) events|=worker::Harvest;
            if((events&worker::Harvest) && goal.mode==CropMode::Retire && expiry<input.hours)
                out.constraints.harvest_deadline[cell]=std::clamp(first_loss,0,input.hours-1);
            if(goal.mode==CropMode::Yield && goal.harvest_age<=dawn.day-tile.planted_day && tile.max_lifespan_step>=0) {
                const int first=tile.max_lifespan_step-dawn.step;
                const int loss_hour=first<0?(-first&1):first;
                const int available=planned_harvest_yield(tile,dawn.day,events);
                out.constraints.harvest_deadline[cell]=std::clamp(loss_hour+2*(available-goal.min_yield),0,input.hours-1);
            }
        }
        input.events[cell]=events;
        if(events&worker::Harvest) {
            const int product=tile.has_animal?int(ANIMALS[tile.what-GOOSE].product):int(tile.what);
            out.harvest_amount[cell]=planned_harvest_yield(tile,dawn.day,events);
            out.field_output[product]+=out.harvest_amount[cell];
        }
        if(events&worker::CollectFertilizer) ++out.field_output[FERTILIZER];
    }
    // Retained producers keep their identities. Unplaced stock reduces purchases
    // but still requires establishment work; targets never count an escape twice.
    for(int species=0;species<3;++species) {
        const int placements=intent.animal_target[species]-retained[species];
        if(placements<0 || placements+input.establish_count>100) return false;
        out.animal_need[species]=std::max(0,placements-input.shed[GOOSE+species]);
        for(int n=0;n<placements;++n) {
            int service=0;
            if(dawn.day<29 && GOOSE+species!=COW && productive_newborns) {
                Tile newborn; newborn.has_animal=true; newborn.what=GOOSE+species; newborn.planted_day=dawn.day;
                service=worker::Feed; ++feeds;
                if(!zero_value_care(newborn,dawn.day)) service|=worker::Care;
            }
            // The target requires next-dawn placement/survival. Deferred newborn
            // service is a distinct fallback with an explicit future-output cost;
            // it never removes requested service from an existing animal.
            if(dawn.day<29 && !service) {
                ++out.newborn_service_deferred[species];
                out.newborn_first_yield_shortfall+=GOOSE+species!=COW && dawn.day+ANIMALS[species].first_yield_day<=29;
            }
            input.establish[input.establish_count++]={uint8_t(GOOSE+species),uint8_t(service)};
        }
    }
    for(int product=0;product<N_CROPS;++product) {
        out.seed_need[product]=std::max(0,intent.new_crops[product]-input.seeds[product]);
        if(input.establish_count+intent.new_crops[product]>100) return false;
        for(int n=0;n<intent.new_crops[product];++n) input.establish[input.establish_count++]={uint8_t(product),worker::Water};
    }
    int sites=intent.buy_land?25:0;
    std::array<int,100> clearable{}; int count=0;
    for(int cell=0;cell<100;++cell) {
        const auto& tile=input.grid[cell]; const int events=input.events[cell];
        if(tile.kind==T_LOCKED || tile.has_animal) continue;
        if(tile.kind!=T_PLANT || (events&worker::Clear) || (!CROPS[tile.what].ongoing && (events&worker::Harvest))) { ++sites; continue; }
        if(intent.crops[cell].mode==CropMode::Retire) clearable[count++]=cell;
    }
    // Retired ongoing crops still occupy their tiles after HARVEST. Derive the
    // missing clearing jobs from establishment counts before asking for movement.
    std::sort(clearable.begin(),clearable.begin()+count,[&](int a,int b) {
        auto cost=[&](int c) {
            const auto& t=input.grid[c];
            const int lost=(input.events[c]&worker::Harvest)?0:int(t.yield_units);
            return std::tuple{lost,shed_distance(c),c};
        };
        return cost(a)<cost(b);
    });
    for(int n=0;n<count && sites<input.establish_count;++n,++sites) input.events[clearable[n]]|=worker::Clear;
    if(sites<input.establish_count) return false;
    // Material balance is only a screen. The worker solver must establish each
    // direct use or timely shed pickup; total field output is not a receipt.
    if(direct_field_inputs) {
        out.field_wheat_used=std::min(out.field_output[WHEAT],std::max(0,feeds-input.shed[WHEAT]));
        out.field_fertilizer_used=std::min(collections,std::max(0,fertilizers-input.shed[FERTILIZER]));
    }
    out.wheat_need=std::max(0,feeds-input.shed[WHEAT]-out.field_wheat_used);
    out.fertilizer_need=std::max(0,fertilizers-input.shed[FERTILIZER]-out.field_fertilizer_used);
    return true;
}
}
