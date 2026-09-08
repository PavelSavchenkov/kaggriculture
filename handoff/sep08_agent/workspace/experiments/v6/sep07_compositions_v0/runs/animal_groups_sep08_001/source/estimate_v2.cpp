#include "season_v2.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_portfolio_001/source/model.hpp"

struct Proposal {
    int cell=0,first=0,item=0,seeds_saved=0,visits=0;
    Biology delta;
    std::array<int,N_ANIMALS> removed{};
};

std::vector<TileWorkAction> work_at(const DayProblem& problem,int cell){
    for(const auto& work:problem.tile_work)if(work.tile==cell)return work.actions;
    return {};
}

void accumulate(Biology& total,const Biology& part){
    total.animal_cost+=part.animal_cost;
    for(int d=0;d<30;++d){
        auto& a=total.days[d];const auto& b=part.days[d];
        a.wheat+=b.wheat;a.fertilizer+=b.fertilizer;a.operations+=b.operations;
        for(int p=0;p<N_PRODUCTS;++p)a.output[p]+=b.output[p];
    }
}

int main(int argc,char** argv){
    if(argc!=4 || fs::exists(argv[1]))return 2;
    const fs::path out(argv[1]);fs::create_directories(out);
    const uint64_t first_seed=std::stoull(argv[2]);const int seeds=std::stoi(argv[3]);
    if(seeds<=0)return 2;
    std::ofstream csv(out/"proposals.csv"),controls(out/"CONTROLS.jsonl");
    csv<<"seed,id,count,entry_day,rotations,animal_cost,removed_animals,seeds_saved,field_operations_delta,source_visit_deficit,mean_own_gain,mean_margin_gain,margin_sd,risk_score,estimate_seconds\n";
    int total=0;
    for(int index=0;index<seeds;++index){
        const uint64_t seed=first_seed+index;const Season source(seed);
        for(const auto& life:source.lives)if(!life.exact){std::cerr<<"inexact crop life "<<life.cell<<'\n';return 3;}
        for(const auto& life:source.animals)if(!life.exact){std::cerr<<"inexact animal life "<<life.cell<<'\n';return 3;}
        Source original;public_router::Agent rival;Options options;
        const auto control=run_game(original,rival,seed,0,options);
        for(int p=0;p<2;++p){
            if(control.hash[p]!=source.hashes[p] || control.cash[p]!=source.final_farms[p].money)std::abort();
            for(int i=0;i<N_ITEMS;++i)if(control.produced[p][i]!=source.final_farms[p].produced[i] ||
                control.sold[p][i]!=source.final_farms[p].sold_units[i] ||
                control.discarded[p][i]!=source.final_farms[p].discarded[i])std::abort();
        }
        controls<<"{\"seed\":"<<seed<<",\"turns\":719,\"crop_lives_exact\":"<<source.lives.size()
            <<",\"animal_lives_exact\":"<<source.animals.size()<<",\"cash\":"<<control.cash[0]<<",\"rival_cash\":"<<control.cash[1]
            <<",\"hash\":\""<<control.hash[0]<<"\",\"rival_hash\":\""<<control.hash[1]<<"\"}\n";
        controls.flush();
        FarmFlowPlan plan;
        std::array<std::array<int,N_ANIMALS>,30> animal_buys{};
        std::array<std::array<int,100>,30> visits{};
        for(int day=0;day<30;++day){
            auto replay=source.days[day].start;
            for(int h=0;h<24 && !replay.st.done;++h){
                const auto& action=source.days[day].own[h];const auto& farm=replay.st.farms[0];
                for(int j=0;j<action.n_orders;++j){
                    const auto& order=action.orders[j];
                    if(order.op==M_BUY_ANIMAL)animal_buys[day][order.item-GOOSE]+=std::max(0,order.n);
                    if(order.op==M_SELL)plan.sales[day][order.item]+=std::max(0,order.n);
                    if(order.op==M_BUY_PRODUCT)plan.buys[day][order.item]+=std::max(0,order.n);
                }
                for(int u=0;u<farm.n_units;++u){
                    const auto op=action.units[u].op;
                    if(op==OP_PASS || (op>=OP_PLANT && op<=OP_CARE))++visits[day][farm.pos_y[u]*10+farm.pos_x[u]];
                }
                replay.step(action,source.days[day].rival[h]);
            }
        }
        int seed_count=0;
        for(int day=8;day<=22;++day){
            std::vector<Proposal> singles;
            for(int cell=0;cell<100;++cell){
                const auto& tile=source.days[day].start.st.farms[0].tiles[cell/10][cell%10];
                if(tile.kind!=T_PLANT || CROPS[tile.what].ongoing)continue;
                const auto original_work=work_at(source.days[day].problem,cell);
                bool released=false;
                for(const auto& job:original_work){if(job.op==OP_PLANT)break;if(job.op==OP_HARVEST)released=true;}
                if(!released)continue;
                for(int item=GOOSE;item<=SHEEP;++item){
                    Service service;service.feed&=~(1u<<29);service.care&=~(1u<<29);
                    const auto& species=ANIMALS[item-GOOSE];
                    int last=day+species.first_yield_day;
                    while(last+species.interval<=29)last+=species.interval;
                    if(last>29)continue;
                    service.care&=(1u<<std::max(0,last-1))-1;
                    Proposal p;p.cell=cell;p.first=day;p.item=item;
                    p.delta=biology({uint8_t(item),1,day,30},service);
                    for(const auto& life:source.lives)if(life.cell==cell && life.cohort.start_day>=day){
                        const auto removed=biology(life.cohort,life.service);p.seeds_saved+=removed.seed_cost;
                        for(int d=day;d<30;++d){
                            for(int product=0;product<N_PRODUCTS;++product)p.delta.days[d].output[product]-=removed.days[d].output[product];
                            p.delta.days[d].output[FERTILIZER]+=removed.days[d].fertilizer;
                            p.delta.days[d].operations-=removed.days[d].operations;
                        }
                    }
                    for(const auto& life:source.animals)if(life.cell==cell && life.cohort.start_day>=day){
                        const auto removed=biology(life.cohort,life.service);++p.removed[life.cohort.item-GOOSE];
                        for(int d=day;d<30;++d){
                            for(int product=0;product<N_PRODUCTS;++product)p.delta.days[d].output[product]-=removed.days[d].output[product];
                            p.delta.days[d].wheat-=removed.days[d].wheat;
                            p.delta.days[d].operations-=removed.days[d].operations;
                        }
                    }
                    for(int d=day;d<30;++d)p.visits+=std::max(0,int(work_at(source.days[d].problem,cell).size())+p.delta.days[d].operations-visits[d][cell]);
                    singles.push_back(p);
                }
            }
            const auto obs=agent::runtime::make_observation(source.days[day].start,0);
            std::array<late_portfolio::Forecast,32> scenarios;
            std::array<InvestmentValue,32> base;
            for(int s=0;s<32;++s){scenarios[s]=late_portfolio::scenario(obs,s);base[s]=value_animal_investment(obs,plan,Biology{},0,.35,&scenarios[s].demand);}
            auto score=[&](const std::vector<int>& members){
                const auto begin=std::chrono::steady_clock::now();
                Biology delta;int saved=0,deficit=0,operations=0,removed_count=0;std::array<int,N_ANIMALS> removed{};std::string encoded;
                for(int id:members){const auto& p=singles[id];accumulate(delta,p.delta);saved+=p.seeds_saved;deficit+=p.visits;
                    for(int i=0;i<N_ANIMALS;++i)removed[i]+=p.removed[i];
                    if(!encoded.empty())encoded+=';';encoded+=std::to_string(p.cell)+":"+std::to_string(day)+":"+std::to_string(p.item);}
                for(int i=0;i<N_ANIMALS;++i){
                    int future_buys=0;for(int d=day;d<30;++d)future_buys+=animal_buys[d][i];
                    delta.animal_cost-=std::min(removed[i],future_buys)*ANIMALS[i].cost;
                    removed_count+=removed[i];
                }
                for(const auto& d:delta.days)operations+=d.operations;
                double own=0,margin=0,square=0;
                for(int s=0;s<32;++s){
                    const auto value=value_animal_investment(obs,plan,delta,0,.35,&scenarios[s].demand);
                    const double m=value.margin()-base[s].margin()+saved;
                    own+=value.own-base[s].own+saved;margin+=m;square+=m*m;
                }
                own/=32;margin/=32;const double sd=std::sqrt(std::max(0.,square/32-margin*margin));
                csv<<seed<<','<<seed_count++<<','<<members.size()<<','<<day<<','<<encoded<<','<<delta.animal_cost<<','<<removed_count<<','<<saved<<','<<operations<<','<<deficit<<','
                    <<own<<','<<margin<<','<<sd<<','<<margin-.5*sd<<','<<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<<'\n';
            };
            for(int a=0;a<int(singles.size());++a){
                score({a});
                for(int b=a+1;b<int(singles.size());++b){
                    if(singles[a].cell==singles[b].cell)continue;
                    score({a,b});
                    for(int c=b+1;c<int(singles.size());++c){
                        if(singles[a].cell==singles[c].cell || singles[b].cell==singles[c].cell)continue;
                        score({a,b,c});
                    }
                }
            }
        }
        total+=seed_count;csv.flush();std::cout<<"seed="<<seed<<" proposals="<<seed_count<<std::endl;
    }
    std::ofstream(out/"STATUS.json")<<"{\"seeds\":"<<seeds<<",\"proposals\":"<<total<<",\"control_games\":"<<seeds*2<<"}\n";
}
