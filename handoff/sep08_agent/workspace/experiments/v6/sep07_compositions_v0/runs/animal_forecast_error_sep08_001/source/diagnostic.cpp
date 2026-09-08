// Offline error attribution only. Actual future shops/flows never enter agents.
#include "experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/source/season_v2.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/late_portfolio_001/source/model.hpp"
#include <iomanip>

struct Trace {
    std::array<FarmFlowPlan,2> flows;
    std::array<double,2> fixed{},cash{};
    std::array<uint64_t,2> hashes{14695981039346656037ULL,14695981039346656037ULL};
    int hire_cost=0;
};
std::array<Action,24> read_actions(const fs::path& path){
    std::ifstream in(path);if(!in)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        in>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;in>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;in>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!in)std::abort();return result;
}
Trace trace(const Season& season,uint64_t seed,int first,const fs::path& folder){
    Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;DetailedProfile profile;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    std::vector<std::array<Action,24>> plans(30);std::array<bool,30> changed{};Trace result;
    if(!folder.empty())for(int d=first;d<30;++d){
        const auto path=folder/"days"/std::to_string(d)/"actions.txt";
        if(!fs::exists(path))std::abort();changed[d]=true;plans[d]=read_actions(path);
    }
    while(!sim.st.done){
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);Action actions[2];
        own.act(agent::runtime::make_observation(sim,0),decision_budget(),actions[0]);
        rival.act(agent::runtime::make_observation(sim,1),decision_budget(),actions[1]);
        if(changed[sim.st.day])actions[0]=plans[sim.st.day][sim.st.hour];
        for(int p=0;p<2;++p){validate_action(actions[p],agent::runtime::make_observation(sim,p));hash_action(result.hashes[p],actions[p]);}
        const auto before=sim;sim.step(actions[0],actions[1]);profile.observe(before,sim,actions);
    }
    if(sim.st.step!=719)std::abort();
    for(int p=0;p<2;++p){
        result.cash[p]=sim.st.farms[p].money;
        for(const auto& flow:profile.farms[p].flows)if(flow.step/24>=first){
            result.flows[p].sales[flow.step/24][flow.item]+=flow.sold;
            result.flows[p].buys[flow.step/24][flow.item]+=flow.bought;
        }
        for(const auto& cost:profile.farms[p].fixed)if(cost.step/24>=first)result.fixed[p]+=cost.cost;
    }
    result.hire_cost=profile.farms[0].hire_cost;return result;
}
void add(Biology& a,const Biology& b){
    a.animal_cost+=b.animal_cost;
    for(int d=0;d<30;++d){a.days[d].wheat+=b.days[d].wheat;a.days[d].operations+=b.days[d].operations;
        for(int p=0;p<N_PRODUCTS;++p)a.days[d].output[p]+=b.days[d].output[p];}
}
std::array<double,2> actual_flow_daily(const kag::agent::AgentObservation& obs,const Trace& trace,const ProductFlows& demand){
    std::array<double,2> value{};
    for(int p=0;p<N_PRODUCTS;++p){
        double stock=obs.market.inventory[p];
        for(int d=obs.day;d<30;++d){
            double sells[2],buys[2];
            for(int seat=0;seat<2;++seat){
                sells[seat]=trace.flows[seat].sales[d][p];buys[seat]=trace.flows[seat].buys[d][p];
                const double internal=std::min(sells[seat],buys[seat]);sells[seat]-=internal;buys[seat]-=internal;
            }
            const double net=sells[0]+sells[1]-buys[0]-buys[1]-demand[d][p];
            const int sell=fractional_quote(p,stock+.5*net),buy=fractional_quote(p,stock+.5*net-1);
            for(int seat=0;seat<2;++seat)value[seat]+=sells[seat]*sell-buys[seat]*buy;
            stock-=demand[d][p]+buys[0]+buys[1];if(sell>1)stock+=sells[0]+sells[1];
        }
    }
    for(int p=0;p<2;++p)value[p]-=trace.fixed[p];return value;
}
void check_case(std::ostream& out,const std::string& name,uint64_t seed,int day,int item,
        const std::array<int,2>& cells,const fs::path& folder,double expected_own,double expected_rival){
    const Season season(seed);const auto before=trace(season,seed,day,{}),after=trace(season,seed,day,folder);
    for(int p=0;p<2;++p)if(before.cash[p]!=season.final_farms[p].money || before.hashes[p]!=season.hashes[p])std::abort();
    if(after.cash[0]!=expected_own || after.cash[1]!=expected_rival)std::abort();
    const auto obs=agent::runtime::make_observation(season.days[day].start,0);
    FarmFlowPlan plan;std::array<int,N_ANIMALS> future_buys{},removed{};
    for(int d=day;d<30;++d)for(const auto& a:season.days[d].own)for(int s=0;s<a.n_orders;++s){
        const auto& m=a.orders[s];
        if(m.op==M_SELL)plan.sales[d][m.item]+=std::max(0,m.n);
        if(m.op==M_BUY_PRODUCT)plan.buys[d][m.item]+=std::max(0,m.n);
        if(m.op==M_BUY_ANIMAL)future_buys[m.item-GOOSE]+=std::max(0,m.n);
    }
    Biology delta;int saved=0;
    for(int cell:cells){
        Service service;service.feed&=~(1u<<29);service.care&=~(1u<<29);
        const auto& species=ANIMALS[item-GOOSE];int last=day+species.first_yield_day;
        while(last+species.interval<=29)last+=species.interval;
        service.care&=(1u<<std::max(0,last-1))-1;
        auto part=biology({uint8_t(item),1,day,30},service);
        for(const auto& life:season.lives)if(life.cell==cell && life.cohort.start_day>=day){
            const auto old=biology(life.cohort,life.service);saved+=old.seed_cost;
            for(int d=day;d<30;++d){
                for(int p=0;p<N_PRODUCTS;++p)part.days[d].output[p]-=old.days[d].output[p];
                part.days[d].output[FERTILIZER]+=old.days[d].fertilizer;part.days[d].operations-=old.days[d].operations;
            }
        }
        for(const auto& life:season.animals)if(life.cell==cell && life.cohort.start_day>=day){
            const auto old=biology(life.cohort,life.service);++removed[life.cohort.item-GOOSE];
            for(int d=day;d<30;++d){
                for(int p=0;p<N_PRODUCTS;++p)part.days[d].output[p]-=old.days[d].output[p];
                part.days[d].wheat-=old.days[d].wheat;part.days[d].operations-=old.days[d].operations;
            }
        }
        add(delta,part);
    }
    for(int i=0;i<N_ANIMALS;++i)delta.animal_cost-=std::min(removed[i],future_buys[i])*ANIMALS[i].cost;
    const int labor=after.hire_cost-before.hire_cost;ProductFlows realized{};
    for(int d=day;d<30;++d)for(int p=0;p<N_PRODUCTS;++p){
        realized[d][p]=p==FERTILIZER?0:1;
        for(int i=0;i<std::min(8,d/3);++i)if(SHOP_MASK[season.shops[i]]&(1u<<p))realized[d][p]+=6*SHOP_MULT[season.shops[i]];
    }
    auto row=[&](const char* stage,double own,double rival){
        out<<name<<','<<seed<<','<<stage<<','<<own<<','<<rival<<','<<own-rival<<','<<labor<<'\n';
    };
    double own=0,rival=0;
    for(int s=0;s<32;++s){
        const auto forecast=late_portfolio::scenario(obs,s);
        const auto a=value_animal_investment(obs,plan,delta,0,.35,&forecast.demand);
        const auto b=value_animal_investment(obs,plan,Biology{},0,.35,&forecast.demand);
        own+=(a.own-b.own+saved)/32;rival+=(a.rival-b.rival)/32;
    }
    row("conditional_before_labor",own,rival);row("conditional_measured_labor",own-labor,rival);
    const auto a=value_animal_investment(obs,plan,delta,0,.35,&realized);
    const auto b=value_animal_investment(obs,plan,Biology{},0,.35,&realized);
    row("actual_shops_measured_labor",a.own-b.own+saved-labor,a.rival-b.rival);
    const auto c=value_animal_investment(obs,after.flows[0],Biology{},0,.35,&realized);
    const auto d=value_animal_investment(obs,before.flows[0],Biology{},0,.35,&realized);
    row("actual_shops_own_flows_fixed_cost",c.own-d.own-after.fixed[0]+before.fixed[0],c.rival-d.rival);
    const auto e=actual_flow_daily(obs,after,realized),f=actual_flow_daily(obs,before,realized);
    row("actual_shops_both_flows_fixed_cost",e[0]-f[0],e[1]-f[1]);
    row("exact_full_game",after.cash[0]-before.cash[0],after.cash[1]-before.cash[1]);
}
int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;std::ofstream out(argv[1]);out<<std::setprecision(12);
    out<<"case,seed,stage,own_gain,rival_gain,margin_gain,extra_labor\n";
    const fs::path root="experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001";
    check_case(out,"early_cow",1000,8,COW,{7,8},root/"broader_cow_audit/cow_pair_d8",84172,75543);
    check_case(out,"early_goose",1001,10,GOOSE,{23,32},root/"broader_goose_audit/goose_pair_d10",83971,71679);
}
