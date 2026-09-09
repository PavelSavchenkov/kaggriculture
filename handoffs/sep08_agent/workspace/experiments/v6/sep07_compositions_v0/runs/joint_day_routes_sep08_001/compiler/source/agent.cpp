#include "agent.hpp"
#include "../../../../include/estimate.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace compositions::joint_day_core {
namespace {
using namespace kag;
#include "../../../../candidates/composition_greedy_v0/source/programs.inc"

int distance(int x,int y,int a,int b) {return std::abs(x-a)+std::abs(y-b);}
int shed_distance(int x,int y) {return std::min(std::abs(x-4),std::abs(x-5))+std::min(std::abs(y-4),std::abs(y-5));}
UnitAction move(int x,int y,int a,int b) {
    if(x!=a)return {uint8_t(x<a?OP_EAST:OP_WEST),0,1};
    if(y!=b)return {uint8_t(y<b?OP_SOUTH:OP_NORTH),0,1};
    return {};
}
bool produces(int item,int born,int day) {
    const auto& a=ANIMALS[item-GOOSE];
    const int age=day-born-a.first_yield_day;
    return age>=0 && age%a.interval==0;
}

struct Job {int x,y;UnitAction action;int input;double value;int deadline=23;int worker=-1;};
struct Jobs {
    std::array<Job,400> data{};int size=0;
    void add(int x,int y,int op,int arg,int input,double value,int deadline=23,int worker=-1,int n=1) {
        if(size==int(data.size()))std::abort();
        data[size++]={x,y,{uint8_t(op),uint8_t(arg),n},input,value,deadline,worker};
    }
};

// A light exact projection for the actions this compiler can emit. No opponent
// private data or future randomness is constructed. It preserves DROP key order.
struct Projection {
    int shed[N_ITEMS]{},inv[MAX_UNITS][N_ITEMS]{},seeds[N_CROPS]{};
    Tile tiles[BOARD][BOARD]{};
    int total=0;
    Projection(const agent::AgentObservation& o,const Action& a) {
        const auto& f=o.self();total=o.own.shed_total;
        std::copy_n(o.own.shed,N_ITEMS,shed);std::copy_n(o.own.seeds,N_CROPS,seeds);
        for(int y=0;y<BOARD;++y)std::copy_n(f.tiles[y],BOARD,tiles[y]);
        for(int u=0;u<f.n_units;++u)std::copy_n(o.own.inv[u],N_ITEMS,inv[u]);
        for(int u=0;u<f.n_units;++u) {
            const auto v=a.units[u];auto& t=tiles[f.pos_y[u]][f.pos_x[u]];
            if(v.op==OP_PICKUP) {int n=std::min(v.n,shed[v.arg]);shed[v.arg]-=n;total-=n;inv[u][v.arg]+=n;}
            if(v.op==OP_DROP) for(int k=0;k<o.own.inv_nkeys[u];++k) {
                int i=o.own.inv_keys[u][k],n=std::min(inv[u][i],std::max(0,100-total));shed[i]+=n;total+=n;inv[u][i]=0;
            }
            if(v.op==OP_PLACE) {
                if(is_animal(v.arg) && !t.has_animal && t.kind==(v.arg==GOOSE?T_COOP:T_PASTURE)) {
                    --inv[u][v.arg];t.has_animal=true;t.what=v.arg;t.planted_day=o.day;
                } else {int n=std::min({v.n,inv[u][v.arg],std::max(0,100-total)});shed[v.arg]+=n;total+=n;inv[u][v.arg]-=n;}
            }
            if(v.op==OP_PLANT) {--seeds[v.arg];t={};t.kind=T_PLANT;t.what=v.arg;t.planted_day=o.day;t.yield_units=CROPS[v.arg].ongoing?0:1;}
            if(v.op==OP_DIG)t={};
            if(v.op==OP_BUILD_COOP)t.kind=T_COOP;
            if(v.op==OP_BUILD_PASTURE)t.kind=T_PASTURE;
            if(v.op==OP_FEED)--inv[u][WHEAT];
            if(v.op==OP_FERTILIZE)--inv[u][FERTILIZER];
            if(v.op==OP_COLLECT_FERTILIZER)++inv[u][FERTILIZER];
            if(v.op==OP_HARVEST) {
                int item=t.kind==T_PLANT?t.what:ANIMALS[t.what-GOOSE].product;
                inv[u][item]+=t.yield_units;t.yield_units=0;
                if(t.kind==T_PLANT && !CROPS[t.what].ongoing)t={};
            }
        }
    }
    int held(int item,int units) const {int n=shed[item];for(int u=0;u<units;++u)n+=inv[u][item];return n;}
};

struct MarketDraft {
    Action& action;Projection& state;double money;int inventory[N_PRODUCTS];int limit;
    MarketDraft(Action& a,Projection& s,const agent::AgentObservation& o,int cap):action(a),state(s),money(o.self().money),limit(cap) {
        std::copy_n(o.market.inventory,N_PRODUCTS,inventory);
    }
    void sell(int item,int count,bool floor=false) {
        if(action.n_orders>=limit || count<=0)return;
        int sold=0;double proceeds=0;
        while(sold<count && state.shed[item]>0) {
            int price=market_price(item,inventory[item]);if(price<=1 && !floor)break;
            proceeds+=price;--state.shed[item];--state.total;++sold;if(price>1)++inventory[item];
        }
        if(sold) {action.orders[action.n_orders++]={M_SELL,uint8_t(item),sold};money+=0.9*proceeds;}
    }
    void buy(int op,int item,int count,int reserve=0) {
        if(action.n_orders>=limit || count<=0)return;
        int bought=0;
        while(bought<count) {
            int price=op==M_BUY_SEED?CROPS[item].seed:op==M_BUY_ANIMAL?ANIMALS[item-GOOSE].cost:market_price(item,inventory[item]-1);
            if(money<price+reserve || (op!=M_BUY_SEED && state.total>=100))break;
            money-=price;++bought;
            if(op==M_BUY_SEED)++state.seeds[item];else {++state.shed[item];++state.total;}
            if(op==M_BUY_PRODUCT)--inventory[item];
        }
        if(bought)action.orders[action.n_orders++]={uint8_t(op),uint8_t(item),bought};
    }
    bool fixed(int op,int cost,int reserve=0) {
        if(action.n_orders>=limit || money<cost+reserve)return false;
        action.orders[action.n_orders++]={uint8_t(op),0,0};money-=cost;return true;
    }
};
}

std::span<const Life> recorded_program(int program) {
    if(program<0 || program>=72)std::abort();
    return {recorded_intents+program_offsets[program],size_t(program_offsets[program+1]-program_offsets[program])};
}

Support recorded_support(int program) {
    if(program<0 || program>=72)std::abort();
    Support result;
    std::copy_n(recorded_hands[program],30,result.hands.begin());
    std::copy_n(recorded_quadrants[program],30,result.quadrants.begin());
    return result;
}

void AgentCore::reset(const kag::agent::AgentInit& init) {
    config_=init.config;
    if(config_.board_size!=10)std::abort();
    if(custom_) {
        intents_=proposal_;hands_=support_.hands;quadrants_=support_.quadrants;
        std::stable_sort(intents_.begin(),intents_.end(),[](const Intent& a,const Intent& b){return a.start<b.start;});
    } else {
        auto program=recorded_program(program_id_);
        intents_.assign(program.begin(),program.end());
        std::copy_n(recorded_hands[program_id_],30,hands_.begin());
        std::copy_n(recorded_quadrants[program_id_],30,quadrants_.begin());
    }
    for(const auto& life:intents_)
        if(life.start<0 || life.start>=life.end || life.end>719 || !(is_crop(life.item) || is_animal(life.item)))std::abort();
    if(!source_layout_) {
        std::array<int,100> release{},previous{};previous.fill(-1);
        int allocated_quadrants=1;
        for(auto& intent:intents_) {
            int best=-1;double score=1e100;
            bool free_owned=false;
            for(int cell=0;cell<100;++cell)
                free_owned|=release[cell]<=intent.start && quadrant_of(cell%10,cell/10,10)<allocated_quadrants;
            for(int cell=0;cell<100;++cell)if(release[cell]<=intent.start) {
                const int x=cell%10,y=cell/10,q=quadrant_of(x,y,10);
                if(placement_mode_ && free_owned && q>=allocated_quadrants)continue;
                const double value=5*q+shed_distance(x,y)*(is_animal(intent.item)?2.0:1.0)
                    -(is_animal(intent.item)&&is_animal(previous[cell])?1.5:0.0)+cell*0.0001;
                if(value<score){score=value;best=cell;}
            }
            if(best<0)std::abort();
            allocated_quadrants=std::max(allocated_quadrants,quadrant_of(best%10,best/10,10)+1);
            intent.x=best%10;intent.y=best/10;release[best]=intent.end;previous[best]=intent.item;
        }
    }
    if(labor_mode_<0 || labor_mode_>3 || placement_mode_<0 || placement_mode_>1 || care_mode_<0 || care_mode_>1)std::abort();
    estimated_hands_.fill(0);
    if(labor_mode_==3) {
        EstimateOptions options;
        options.recorded_layout=true;
        options.service=source_service_?ServiceModel::Recorded:ServiceModel::Productive;
        estimated_hands_=estimate_plan(intents_,support_,options).hands;
    }
}

void AgentCore::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    MarketContext context;
    plan_units(o,action,context);
    plan_market(o,action,context);
}

void AgentCore::plan_units(const kag::agent::AgentObservation& o,kag::Action& action,MarketContext& context) {
    using namespace kag;
    const auto& farm=o.self();const auto& own=o.own;
    action.clear();action.n_units=farm.n_units;std::fill_n(action.units,action.n_units,UnitAction{});
    std::array<int,100> desired,existing;auto& soon=context.soon;desired.fill(-1);soon.fill(-1);existing.fill(-1);
    for(int i=0;i<int(intents_.size());++i) {
        const auto& in=intents_[i];const int cell=in.y*10+in.x;
        const auto& tile=farm.tiles[in.y][in.x];
        if((tile.kind==T_PLANT || tile.has_animal) && tile.what==in.item && tile.planted_day>=in.start/24 && tile.planted_day*24<in.end)existing[cell]=i;
        // The proposal fixes the birth day, not its recorded worker's hour.
        // Same-day replacement is handled by harvesting/clearing the old tile.
        if(in.start/24<=o.day && o.step<in.end)desired[cell]=i;
        if((in.start/24<=o.day || in.start<=std::min(718,o.step+12)) && o.step<in.end)soon[cell]=i;
    }
    int feed_needed=0,fertilizer_needed=0,animal_jobs[N_ANIMALS]{};
    Jobs jobs;
    for(int y=0;y<10;++y)for(int x=0;x<10;++x) {
        const int cell=y*10+x;const auto& t=farm.tiles[y][x];
        const Intent* in=desired[cell]<0?nullptr:&intents_[desired[cell]];
        const Intent* service=source_service_ && existing[cell]>=0?&intents_[existing[cell]]:nullptr;
        const uint32_t today=uint32_t{1}<<o.day;
        if(t.kind==T_LOCKED)continue;
        if(t.has_animal) {
            const auto& rule=ANIMALS[t.what-GOOSE];
            const int end_day=in?std::min(29,(in->end-1)/24):o.day;
            const bool retiring=!in || (in->end<719 && o.day>=end_day-1);
            const bool productive=o.day<29 && !retiring;
            if(t.yield_units>0 && (!service || (service->harvest&today) || o.day==29 || t.yield_units>=rule.max_held))
                jobs.add(x,y,OP_HARVEST,0,-1,600+t.yield_units*o.market.prices[rule.product]);
            if(t.fertilizer_available && (!service || (service->collect&today) || o.day==29))
                jobs.add(x,y,OP_COLLECT_FERTILIZER,0,-1,400+o.market.prices[FERTILIZER]);
            if(productive && !t.fed_today && (!service || (service->feed&today) || t.consecutive_dry>=1)) {
                ++feed_needed;jobs.add(x,y,OP_FEED,0,WHEAT,(t.consecutive_dry?7000:1200),23);
            }
            bool future_bonus=false;
            for(int day=o.day+2;day<=end_day;++day)future_bonus|=produces(t.what,t.planted_day,day);
            if(productive && !t.cared_today && future_bonus && (t.pending_care_bonus<rule.max_held-1 || (care_mode_ && produces(t.what,t.planted_day,o.day+1))) && (!service || (service->care&today)))
                jobs.add(x,y,OP_CARE,0,-1,800+o.market.prices[rule.product]);
            continue;
        }
        if(t.kind==T_PLANT) {
            const auto& rule=CROPS[t.what];const int age=o.day-t.planted_day;
            const bool mismatch=in && (in->item!=t.what || t.planted_day<in->start/24);
            const bool ripe=age>=rule.first_yield_day && t.yield_units>0;
            const int deadline=in?std::min(29,(in->end-1)/24):29;
            bool tonight=false;
            if(rule.ongoing) {int since=age+1-rule.first_yield_day;tonight=since>=0 && since%rule.interval==0 && since/rule.interval<rule.max_yield;}
            const bool growth=!rule.ongoing && age>=(rule.max_yield_day+1)/2 && age<=rule.max_yield_day;
            const bool harvest=ripe && (mismatch || (service ? bool(service->harvest&today) : (rule.ongoing || age>=rule.max_yield_day)) || o.day>=deadline || o.day==29);
            if(mismatch && !ripe) {jobs.add(x,y,OP_DIG,0,-1,1500);continue;}
            const bool want_fert=service ? bool(service->fertilize&today) && t.fertilized_until_day<o.day+2
                : (t.what==STRAWBERRY || t.what==TOMATO) && tonight && t.fertilized_until_day<o.day;
            if(want_fert && o.day<29) {
                ++fertilizer_needed;
                int available=own.shed[FERTILIZER];for(int u=0;u<farm.n_units;++u)available+=own.inv[u][FERTILIZER];
                if(available)jobs.add(x,y,OP_FERTILIZE,0,FERTILIZER,(service?7200:1300)+o.market.prices[t.what]);
            }
            const bool scheduled_water=service ? bool(service->water&today) : (t.consecutive_dry>=1 || growth || tonight);
            const bool fertilizer_first=service && want_fert && growth && t.fertilized_until_day<o.day && o.hour<20;
            const bool water=!t.watered_today && scheduled_water && !fertilizer_first;
            if(water && !(harvest && (!growth || t.yield_units>=rule.max_yield)))
                jobs.add(x,y,OP_WATER,0,-1,t.consecutive_dry>=1?6000:1400);
            if(harvest && !(growth && scheduled_water && !t.watered_today && t.yield_units<rule.max_yield) && !fertilizer_first)
                jobs.add(x,y,OP_HARVEST,0,-1,1000+t.yield_units*o.market.prices[t.what]);
            continue;
        }
        if(!in || o.day==29)continue;
        if(t.kind==T_WEED) {jobs.add(x,y,OP_DIG,0,-1,600);continue;}
        if(is_animal(in->item)) {
            const auto& rule=ANIMALS[in->item-GOOSE];
            if(o.day+rule.first_yield_day>=30)continue;
            const auto kind=in->item==GOOSE?T_COOP:T_PASTURE;
            if(t.kind==T_EMPTY)jobs.add(x,y,in->item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE,0,-1,850);
            else if(t.kind!=kind)jobs.add(x,y,OP_DIG,0,-1,650);
            else {++animal_jobs[in->item-GOOSE];jobs.add(x,y,OP_PLACE,in->item,in->item,1500);}
        } else {
            if(o.day+CROPS[in->item].first_yield_day>std::min(29,(in->end-1)/24))continue;
            if(t.kind!=T_EMPTY)jobs.add(x,y,OP_DIG,0,-1,600);
            else if(own.seeds[in->item]>0 && o.hour<22)
                jobs.add(x,y,OP_PLANT,in->item,-1,800+0.4*CROPS[in->item].max_yield*o.market.prices[in->item],21);
        }
    }
    for(int item : {int(WHEAT),int(FERTILIZER),int(GOOSE),int(COW),int(SHEEP)}) {
        int need=item==WHEAT?feed_needed:item==FERTILIZER?fertilizer_needed:animal_jobs[item-GOOSE];
        for(int u=0;u<farm.n_units;++u)need-=own.inv[u][item];
        int remaining=std::min(std::max(0,need),int(own.shed[item]));
        const int bundle=is_animal(item)?1:4;
        for(int u=0;u<farm.n_units && remaining>0;++u) {
            int n=std::min(bundle,remaining);remaining-=n;
            jobs.add(-1,-1,OP_PICKUP,item,-1,item==WHEAT?1700:(source_service_&&item==FERTILIZER?6500:1500),22,-1,n);
        }
    }
    for(int u=0;u<farm.n_units;++u) {
        int best=-1;double value=0;int total=0;bool animals=false;
        for(int item=0;item<N_ITEMS;++item) {
            int n=own.inv[u][item];total+=n;animals|=is_animal(item)&&n>0;
            if(item<N_PRODUCTS && n>0 && !(item==WHEAT && feed_needed>0) && !(item==FERTILIZER && fertilizer_needed>0)) {
                value+=n*o.market.prices[item];
                if(best<0 || n*o.market.prices[item]>own.inv[u][best]*o.market.prices[best])best=item;
            }
        }
        if(best<0)continue;
        const bool terminal=o.day==29;
        const int dist=shed_distance(farm.pos_x[u],farm.pos_y[u]);
        const bool needed=terminal || farm.money<500 || total>=15 || value>=1500 || (dist==0 && value>=200);
        if(!needed)continue;
        const bool protected_stock=animals || (own.inv[u][WHEAT]>0 && feed_needed>0) || (own.inv[u][FERTILIZER]>0 && fertilizer_needed>0);
        double score=(terminal?3000:600)+0.4*value;
        if(terminal && 718-o.step<=dist+2)score+=10000;
        jobs.add(-1,-1,protected_stock?OP_PLACE:OP_DROP,best,-1,score,23,u,protected_stock?own.inv[u][best]:1);
    }
    context.task_count.fill(0);context.sites.fill(false);
    for(int cell=0;cell<100;++cell) {
        const auto& t=farm.tiles[cell/10][cell%10];
        context.sites[cell]=desired[cell]>=0 || t.kind==T_PLANT || t.has_animal;
    }
    for(int j=0;j<jobs.size;++j) {
        const auto& job=jobs.data[j];if(job.x<0)continue;
        int cell=job.y*10+job.x,n=context.task_count[cell]++;
        if(n>=6)std::abort();
        context.tasks[cell][n]={job.action,job.input,job.value,job.deadline};
        context.sites[cell]=true;
    }
    struct Pair {double score;int worker,job,x,y,travel;};
    std::array<Pair,MAX_UNITS*400> pairs;int count=0;
    for(int u=0;u<farm.n_units;++u)for(int j=0;j<jobs.size;++j) {
        const auto& job=jobs.data[j];if(job.worker>=0 && job.worker!=u)continue;
        if(job.input>=0 && own.inv[u][job.input]<=0)continue;
        if(job.action.op==OP_PICKUP && own.inv[u][job.action.arg]>0)continue;
        int x=job.x<0?std::clamp<int>(farm.pos_x[u],4,5):job.x;
        int y=job.y<0?std::clamp<int>(farm.pos_y[u],4,5):job.y;
        int travel=distance(farm.pos_x[u],farm.pos_y[u],x,y);
        if(o.hour+travel>job.deadline)continue;
        if(o.day==29 && job.x>=0 && travel+1+shed_distance(x,y)+1>719-o.step)continue;
        double score=job.value/(1+0.3*travel)+(travel==0?200:0);
        pairs[count++]={score,u,j,x,y,travel};
    }
    std::sort(pairs.begin(),pairs.begin()+count,[](const Pair& a,const Pair& b){if(a.score!=b.score)return a.score>b.score;if(a.worker!=b.worker)return a.worker<b.worker;return a.job<b.job;});
    bool used_workers[MAX_UNITS]{},used_jobs[400]{},used_cells[100]{};int seeds[N_CROPS],pickups[N_ITEMS];
    std::copy_n(own.seeds,N_CROPS,seeds);std::copy_n(own.shed,N_ITEMS,pickups);
    for(int i=0;i<count;++i) {
        const auto& p=pairs[i];const auto& job=jobs.data[p.job];
        if(used_workers[p.worker] || used_jobs[p.job] || (job.x>=0 && used_cells[job.y*10+job.x]))continue;
        auto selected=job.action;
        if(p.travel)selected=move(farm.pos_x[p.worker],farm.pos_y[p.worker],p.x,p.y);
        else if(selected.op==OP_PLANT) {if(seeds[selected.arg]<=0)continue;--seeds[selected.arg];}
        else if(selected.op==OP_PICKUP) {selected.n=std::min(selected.n,pickups[selected.arg]);if(selected.n<=0)continue;pickups[selected.arg]-=selected.n;}
        action.units[p.worker]=selected;used_workers[p.worker]=used_jobs[p.job]=true;
        if(job.x>=0)used_cells[job.y*10+job.x]=true;
    }
    context.feed_needed=feed_needed;context.fertilizer_needed=fertilizer_needed;context.job_count=jobs.size;
}

// Project the actual supplied unit actions before choosing any market order.
// Alternative physical planners must preserve unit count and legal actions.
void AgentCore::plan_market(const kag::agent::AgentObservation& o,kag::Action& action,const MarketContext& context) {
    using namespace kag;
    const auto& farm=o.self();const auto& soon=context.soon;
    const int feed_needed=context.feed_needed,fertilizer_needed=context.fertilizer_needed;
    Projection projected(o,action);
    int target_animals[N_ANIMALS]{},current_animals[N_ANIMALS]{},seed_need[N_CROPS]{};
    int required_quadrants=1;
    for(int cell=0;cell<100;++cell) {
        const auto& t=projected.tiles[cell/10][cell%10];if(t.has_animal)++current_animals[t.what-GOOSE];
        if(soon[cell]<0)continue;
        const auto& in=intents_[soon[cell]];required_quadrants=std::max(required_quadrants,quadrant_of(in.x,in.y,10)+1);
        if(is_animal(in.item))++target_animals[in.item-GOOSE];
        else if(!(t.kind==T_PLANT && t.what==in.item && t.planted_day>=in.start/24) && o.day+CROPS[in.item].first_yield_day<=std::min(29,(in.end-1)/24))++seed_need[in.item];
    }
    MarketDraft market(action,projected,o,config_.max_orders);
    int herd=std::accumulate(current_animals,current_animals+3,0);
    const int target_herd=std::max(herd,std::accumulate(target_animals,target_animals+3,0));
    // Expansion inputs must use the same herd target on both sides of the
    // market. Otherwise an unplaced animal causes repeated sell/buy churn.
    const int wheat_reserve=o.day==29?0:std::max(feed_needed,
        reserve_days_?target_herd*std::min(reserve_days_,29-o.day):herd);
    const int fertilizer_reserve=o.day==29?0:fertilizer_needed;
    std::array<int,N_PRODUCTS> sale_order{};std::iota(sale_order.begin(),sale_order.end(),0);
    std::sort(sale_order.begin(),sale_order.end(),[&](int a,int b){return o.market.prices[a]*projected.shed[a]>o.market.prices[b]*projected.shed[b];});
    for(int item:sale_order) {
        int reserve=item==WHEAT?wheat_reserve:item==FERTILIZER?fertilizer_reserve:0;
        market.sell(item,std::min(projected.shed[item],std::max(0,projected.held(item,farm.n_units)-reserve)),o.day==29 || projected.total>=90);
    }
    if(o.day<29) {
        const int target_land=source_support_?quadrants_[o.day]:required_quadrants;
        for(int n=farm.n_quadrants;n<target_land;++n)if(!market.fixed(M_BUY_LAND,LAND_PRICES[n-1],100))break;
        int target_hands=source_support_?hands_[o.day]:std::clamp(int(std::ceil(context.job_count*2.3/22))-1,2,16);
        if(labor_mode_==1)target_hands=8;
        if(labor_mode_==2)target_hands=10;
        if(labor_mode_==3)target_hands=std::max(target_hands,estimated_hands_[o.day]);
        if(o.hour<=3)for(int n=farm.hires_today;n<target_hands;++n)if(!market.fixed(M_HIRE,fib(n),flexible_hiring_?0:60))break;
        for(int item : {int(MELON),int(WHEAT),int(STRAWBERRY),int(CARROT),int(TOMATO)})
            market.buy(M_BUY_SEED,item,std::max(0,seed_need[item]-projected.seeds[item]),60);
        for(int ai=0;ai<N_ANIMALS;++ai) {
            int item=GOOSE+ai;
            if(o.day+ANIMALS[ai].first_yield_day>=30)continue;
            market.buy(M_BUY_ANIMAL,item,std::max(0,target_animals[ai]-current_animals[ai]-projected.held(item,farm.n_units)),100);
        }
        market.buy(M_BUY_PRODUCT,WHEAT,std::max(0,target_herd-projected.held(WHEAT,farm.n_units)),0);
        market.buy(M_BUY_PRODUCT,FERTILIZER,std::max(0,fertilizer_needed-projected.held(FERTILIZER,farm.n_units)),40);
    } else if(o.hour<=2) {
        int target=source_support_?hands_[o.day]:std::min(12,std::max(2,context.job_count/3));
        if(labor_mode_==1)target=8;
        if(labor_mode_==2)target=10;
        if(labor_mode_==3)target=std::max(target,estimated_hands_[o.day]);
        for(int n=farm.hires_today;n<target;++n)if(!market.fixed(M_HIRE,fib(n)))break;
    }
    action.finalize();
}
}
