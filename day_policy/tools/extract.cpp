#include "case.hpp"
using contract_benchmark::Case;
#include "verify.hpp"
#include "trace/case.hpp"
#include "trace/field_effect.hpp"
#include <bit>
#include <iostream>
#include <map>
using namespace kag;
using namespace kag::agents::day_policy_contract;
using namespace schedule_search;

int event_of(int op) {
    switch(op) {
        case OP_WATER:return Water;case OP_FERTILIZE:return Fertilize;case OP_HARVEST:return Harvest;
        case OP_DIG:return Clear;case OP_FEED:return Feed;case OP_CARE:return Care;case OP_COLLECT_FERTILIZER:return CollectFertilizer;
        default:return 0;
    }
}
void save(std::ofstream& file,const Case& c) {file.write(reinterpret_cast<const char*>(&c),sizeof(c));}
int main(int argc,char** argv) {
    if(argc!=4)throw std::runtime_error("extract cohort.txt cases.bin exclusions.csv");
    std::ifstream cohort(argv[1]);std::ofstream binary(argv[2],std::ios::binary),csv(argv[3]);
    csv<<"game,seat,rank,day,reason,original_hires,work,return_units\n";
    int game_id,seat,rank;std::string path;std::map<std::string,int> reasons;int total=0;
    while(cohort>>game_id>>seat>>rank>>path) {
        auto game=load_case(path);auto states=validate_case(game);
        for(int day=0;day<30;++day) {
            Case c; c.game=game_id;c.seat=seat;c.rank=rank;c.day=day;
            auto& in=c.input;const auto& dawn=states[day*24].st.farms[seat];
            auto reject=[&](const char* reason){if(!c.reason[0])std::snprintf(c.reason,sizeof(c.reason),"%s",reason);};
            if(day==29)reject("short_final_day");
            for(int cell=0;cell<100;++cell) {
                auto t=dawn.tiles[cell/10][cell%10];t.planted_day-=day;t.fertilized_until_day-=day;
                t.max_lifespan_step=t.max_lifespan_step<0?INT_MAX:t.max_lifespan_step-day*24;in.grid[cell]=t;
            }
            for(int it=0;it<N_ITEMS;++it)in.shed[it]=dawn.shed[it];
            for(int it=0;it<N_CROPS;++it)in.seeds[it]=dawn.seeds[it];
            int label[100],built[100]{},tags[MAX_UNITS][N_PRODUCTS]{},receipts[N_PRODUCTS]{};
            for(int cell=0;cell<100;++cell)label[cell]=cell;
            for(int h=0;h<24 && day*24+h<719;++h) {
                const int step=day*24+h;const auto& sim=states[step];const auto& f=sim.st.farms[seat];
                auto accepted=sim.sanitize_joint_actions(game.turns[step].actions[0],game.turns[step].actions[1]);
                auto field_sim=sim;auto actions=game.turns[step];for(auto& a:actions.actions)a.n_orders=0;
                if(h==23)field_sim.st.hour=22;
                field_sim.step(actions.actions[0],actions.actions[1]);
                const auto& after=field_sim.st.farms[seat];
                int shed_total=f.shed_total;
                Tile tiles[100];std::copy_n(&f.tiles[0][0],100,tiles);
                for(int u=0;u<f.n_units;++u) {
                    auto a=accepted[seat].units[u];const int cell=f.pos_y[u]*10+f.pos_x[u];const int g=label[cell];
                    if(a.op==OP_PICKUP && a.arg==FERTILIZER)reject("fertilizer_pickup");
                    const bool establish=a.op==OP_PLANT || (a.op==OP_PLACE && is_animal(a.arg) &&
                        !tiles[cell].has_animal && tiles[cell].kind==(ANIMALS[a.arg-GOOSE].structure==ST_COOP?T_COOP:T_PASTURE));
                    if(establish) {
                        if(g>=100)reject("repeated_new_generation");
                        if(in.establish_count==100)throw std::runtime_error("too many establishments");
                        label[cell]=100+in.establish_count;in.establish[in.establish_count++]={a.arg,0};
                        if(in.grid[cell].kind==T_WEED || (!in.grid[cell].has_animal && (in.grid[cell].kind==T_COOP || in.grid[cell].kind==T_PASTURE)))
                            in.events[cell]&=~Clear; // Preparation belongs to placement, not a fixed-tile job.
                    } else {
                        const int event=event_of(a.op);
                        if(event) {
                            auto& events=g<100?in.events[g]:in.establish[g-100].events;
                            if(events&event)reject("repeated_event");
                            events|=event;
                        }
                    }
                    if(a.op==OP_BUILD_COOP || a.op==OP_BUILD_PASTURE)built[cell]=1;
                    int actual_deposits[N_PRODUCTS]{};
                    if(a.op==OP_PICKUP) shed_total-=int(after.inv[u][a.arg])-f.inv[u][a.arg];
                    if(a.op==OP_DROP) {
                        for(int k=0;k<f.inv_nkeys[u];++k) {
                            const int it=f.inv_keys[u][k], take=std::min<int>(f.inv[u][it],std::max(0,sim.cfg.shed_capacity-shed_total));
                            shed_total+=take; if(it<N_PRODUCTS)actual_deposits[it]=take;
                        }
                    } else if(a.op==OP_PLACE && !establish) {
                        const int take=int(f.inv[u][a.arg])-after.inv[u][a.arg];
                        shed_total+=take;if(a.arg<N_PRODUCTS)actual_deposits[a.arg]=take;
                    }
                    for(int it=0;it<N_PRODUCTS;++it) {
                        const int delta=int(after.inv[u][it])-f.inv[u][it];
                        if((a.op==OP_HARVEST || a.op==OP_COLLECT_FERTILIZER) && delta>0)tags[u][it]+=delta;
                        if((a.op==OP_DROP || (a.op==OP_PLACE && !establish)) && delta<0) {
                            const int returned=std::min(tags[u][it],actual_deposits[it]);receipts[it]+=returned;tags[u][it]=std::min(tags[u][it]-returned,int(after.inv[u][it]));
                        }
                        if((a.op==OP_FEED || a.op==OP_FERTILIZE) && delta<0)
                            tags[u][it]=std::min(tags[u][it],int(after.inv[u][it])); // Consume purchased stock first.
                    }
                    field_effect(tiles[cell],a);
                }
                for(int k=0;k<accepted[seat].n_orders;++k) {
                    const auto a=accepted[seat].orders[k];
                    if(a.op==M_BUY_SEED)in.buy_seeds[a.item]+=a.n;
                    if(a.op==M_BUY_ANIMAL)in.buy_animals[a.item-GOOSE]+=a.n;
                    if(a.op==M_BUY_PRODUCT) {if(a.item==FERTILIZER)reject("fertilizer_purchase");else in.buy_wheat[h]+=a.n;}
                    if(a.op==M_BUY_LAND) {if(in.land_hour>=0)reject("multiple_land_purchases");in.land_hour=h;}
                    if(a.op==M_HIRE)c.original_hires+=1;
                }
                for(int it=0;it<N_PRODUCTS;++it)in.returns[h][it]=receipts[it];
            }
            for(int cell=0;cell<100;++cell) {
                if(built[cell] && label[cell]<100)reject("housing_without_establishment");
                c.work+=std::popcount(in.events[cell]);
            }
            for(int n=0;n<in.establish_count;++n) {
                const auto e=in.establish[n];
                if(e.events & ~(is_crop(e.product)?Water|Fertilize:Feed|Care))reject("unsupported_new_events");
                c.work+=1+std::popcount(e.events);
            }
            for(int it=0;it<N_PRODUCTS;++it)c.early+=in.returns[22][it];
            if(!c.reason[0] && !detail::valid_input(in))reject("invalid_contract_input");
            if(!c.reason[0])std::snprintf(c.reason,sizeof(c.reason),"eligible");
            ++reasons[c.reason];++total;
            csv<<game_id<<','<<seat<<','<<rank<<','<<day<<','<<c.reason<<','<<c.original_hires<<','<<c.work<<','<<c.early<<'\n';
            save(binary,c);
        }
        std::cout<<game_id<<" seat="<<seat<<" rank="<<rank<<" days="<<total<<'\n'<<std::flush;
    }
    for(const auto& [reason,count]:reasons)std::cout<<reason<<' '<<count<<'\n';
}
