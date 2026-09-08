#include "agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "experiments/v6/sep07_compositions_v0/include/profile.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace kag;
using kag::agent::AgentObservation;

static bool read(std::istream& in,AgentObservation& o,Action& a){
    int reset,player;if(!(in>>reset>>player>>o.step>>o.day>>o.hour))return false;o.player=player;
    for(auto&f:o.farms){
        in>>f.money>>f.n_units>>f.n_quadrants>>f.hires_today;
        for(int u=0;u<f.n_units;++u){int x,y;in>>x>>y;f.pos_x[u]=x;f.pos_y[u]=y;}
        for(auto&row:f.tiles)for(auto&t:row){
            int kind,what,animal,water,fed,care,fert,dry,yield,bonus,planted,maxlife,until;
            in>>kind>>what>>animal>>water>>fed>>care>>fert>>dry>>yield>>bonus>>planted>>maxlife>>until;
            t.kind=TileKind(kind);t.what=what;t.has_animal=animal;t.watered_today=water;t.fed_today=fed;t.cared_today=care;t.fertilizer_available=fert;t.consecutive_dry=dry;t.yield_units=yield;t.pending_care_bonus=bonus;t.planted_day=planted;t.max_lifespan_step=maxlife;t.fertilized_until_day=until;
        }
    }
    for(auto&v:o.own.shed){int n;in>>n;v=n;o.own.shed_total+=n;}for(auto&v:o.own.seeds){int n;in>>n;v=n;}
    for(int u=0;u<o.self().n_units;++u){int n;in>>n;o.own.inv_nkeys[u]=n;for(int k=0;k<n;++k){int item,q;in>>item>>q;o.own.inv_keys[u][k]=item;o.own.inv[u][item]=q;}}
    for(auto&v:o.market.prices)in>>v;for(auto&v:o.market.inventory)in>>v;in>>o.n_shops;for(int s=0;s<o.n_shops;++s){int v;in>>v;o.shops[s]=v;}
    a.clear();in>>a.n_units>>a.n_orders;for(int u=0;u<a.n_units;++u){int op,arg,n;in>>op>>arg>>n;a.units[u]={uint8_t(op),uint8_t(arg),n};}for(int i=0;i<a.n_orders;++i){int op,item,n;in>>op>>item>>n;a.orders[i]={uint8_t(op),uint8_t(item),n};}a.finalize();return bool(in);
}

static std::vector<long long> state(const AgentObservation&o){
    const auto&f=o.self();std::vector<long long> v={static_cast<long long>(f.money),f.n_units,f.n_quadrants,f.hires_today};
    for(int u=0;u<f.n_units;++u){v.push_back(f.pos_x[u]);v.push_back(f.pos_y[u]);}
    for(const auto&row:f.tiles)for(const auto&t:row){
        v.push_back(t.kind);
        if(t.kind==T_EMPTY||t.kind==T_LOCKED){for(int i=0;i<12;++i)v.push_back(0);continue;}
        for(int x:{int(t.kind==T_PLANT||t.has_animal?t.what:0),int(t.has_animal),int(t.watered_today),int(t.fed_today),int(t.cared_today),int(t.fertilizer_available),int(t.consecutive_dry),int(t.yield_units),int(t.pending_care_bonus),int(t.planted_day),int(t.max_lifespan_step),int(t.fertilized_until_day)})v.push_back(x);
    }
    for(int x:o.own.shed)v.push_back(x);for(int x:o.own.seeds)v.push_back(x);
    for(int u=0;u<f.n_units;++u)for(int x:o.own.inv[u])v.push_back(x);
    for(int x:o.market.inventory)v.push_back(x);for(int x:o.market.prices)v.push_back(x);return v;
}

static bool same(const Action&a,const Action&b){
    if(a.n_units!=b.n_units||a.n_orders!=b.n_orders)return false;
    for(int u=0;u<a.n_units;++u){const auto&x=a.units[u];const auto&y=b.units[u];if(x.op!=y.op)return false;if((x.op==OP_PICKUP||x.op==OP_PLACE)&&(x.arg!=y.arg||x.n!=y.n))return false;if(x.op==OP_PLANT&&x.arg!=y.arg)return false;}
    for(int i=0;i<a.n_orders;++i){const auto&x=a.orders[i];const auto&y=b.orders[i];if(x.op!=y.op)return false;if(x.op!=M_NONE&&x.op!=M_HIRE&&x.op!=M_BUY_LAND&&(x.item!=y.item||x.n!=y.n))return false;}return true;
}

int main(int argc,char**argv){
    if(argc!=5)return 2;std::ifstream in(argv[1]);const int branch=std::stoi(argv[2]),seat=std::stoi(argv[3]);std::ofstream out(argv[4]);
    Config cfg;cfg.weed_chance=0;cfg.seed=0;Sim sim(cfg);compositions::DetailedProfile profile;compositions::sheep_portfolio::Agent policy(branch);policy.reset(kag::agent::runtime::make_agent_init(sim,seat));
    int first=-1,firstseat=-1,firstindex=-1,actionfirst=-1,weedcount=0,badstates=0;long long actualfirst=0,expectedfirst=0;double cash[2]={},truthcash[2]={};
    out<<"{\"branch\":"<<branch<<",\"seat\":"<<seat;
    for(int step=0;step<720;++step){
        AgentObservation expected[2]{};Action actions[2];if(!read(in,expected[0],actions[0])||!read(in,expected[1],actions[1]))return 3;
        sim.st.n_shops=expected[0].n_shops;std::copy_n(expected[0].shops,sim.st.n_shops,sim.st.shops);
        if(step&&step%24==0)for(int p=0;p<2;++p)for(int y=0;y<BOARD;++y)for(int x=0;x<BOARD;++x){auto&f=sim.st.farms[p];if(f.tiles[y][x].kind==T_EMPTY&&expected[p].self().tiles[y][x].kind==T_WEED){f.tiles[y][x]=Tile{};f.tiles[y][x].kind=T_WEED;const int bit=y*BOARD+x;f.empty_mask[bit/64]&=~(uint64_t(1)<<(bit%64));++weedcount;}}
        for(int p=0;p<2;++p){
            const auto a=state(kag::agent::runtime::make_observation(sim,p)),e=state(expected[p]);
            if(a!=e){++badstates;if(first<0){first=step;firstseat=p;size_t i=0;while(i<a.size()&&i<e.size()&&a[i]==e[i])++i;firstindex=i;actualfirst=i<a.size()?a[i]:-999999;expectedfirst=i<e.size()?e[i]:-999999;out<<",\"first_actual_state\":[";for(size_t k=0;k<a.size();++k){if(k)out<<",";out<<a[k];}out<<"],\"first_expected_state\":[";for(size_t k=0;k<e.size();++k){if(k)out<<",";out<<e[k];}out<<"]";}}
            cash[p]=sim.st.farms[p].money;truthcash[p]=expected[p].self().money;
        }
        if(step<719){Action emitted;policy.act(kag::agent::runtime::make_observation(sim,seat),{},emitted);if(!same(emitted,actions[seat])&&actionfirst<0)actionfirst=step;actions[seat]=emitted;const auto before=sim;sim.step(actions[0],actions[1]);profile.observe(before,sim,actions);}
    }
    out<<",\"first_state_mismatch\":"<<first<<",\"first_mismatch_seat\":"<<firstseat<<",\"first_mismatch_vector_index\":"<<firstindex<<",\"actual_value\":"<<actualfirst<<",\"expected_value\":"<<expectedfirst<<",\"first_action_mismatch\":"<<actionfirst<<",\"mismatched_seat_states\":"<<badstates<<",\"injected_day_start_weeds\":"<<weedcount<<",\"final_cash\":["<<cash[0]<<","<<cash[1]<<"],\"donor_final_cash\":["<<truthcash[0]<<","<<truthcash[1]<<"]";
    const auto&farm=sim.st.farms[seat];for(const auto&entry:std::vector<std::pair<const char*,const int32_t*>>{{"produced",farm.produced},{"sold",farm.sold_units},{"discarded",farm.discarded}}){out<<",\""<<entry.first<<"\":[";for(int i=0;i<N_ITEMS;++i){if(i)out<<",";out<<entry.second[i];}out<<"]";}
    out<<",\"profile\":";profile.write(out,seat);out<<"}\n";
    std::cout<<"branch="<<branch<<" first_state="<<first<<" seat="<<firstseat<<" index="<<firstindex<<" cpp="<<actualfirst<<" donor="<<expectedfirst<<" first_action="<<actionfirst<<"\n";
}
