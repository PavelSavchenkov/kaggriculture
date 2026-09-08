#pragma once
#include "../joint_day_routes_sep08_001/compiler/source/agent.hpp"
#include <algorithm>
#include <array>

namespace compositions::joint_resource_routes {
using namespace kag;
using Context=joint_day_core::AgentCore::MarketContext;
inline int distance(int x,int y,int a,int b){return std::abs(x-a)+std::abs(y-b);}
inline int shed_distance(int x,int y){return distance(x,y,std::clamp(x,4,5),std::clamp(y,4,5));}
inline UnitAction toward(int x,int y,int a,int b) {
    if(x!=a)return {uint8_t(x<a?OP_EAST:OP_WEST),0,1};
    if(y!=b)return {uint8_t(y<b?OP_SOUTH:OP_NORTH),0,1};
    return {};
}

// Required initial stock follows route order: local harvest/collection may
// supply later input tasks, but later output cannot fund an earlier task.
inline std::array<int,N_ITEMS> required_inputs(const agent::AgentObservation& o,const Context& c,
        const std::array<int,100>& route,int size,int start=0) {
    std::array<int,N_ITEMS> balance{},required{};
    for(int offset=0;offset<size;++offset) {
        int cell=route[(start+offset)%size];
        for(int j=0;j<c.task_count[cell];++j) {
            const auto& task=c.tasks[cell][j];
            if(task.action.op==OP_COLLECT_FERTILIZER)++balance[FERTILIZER];
            if(task.action.op==OP_HARVEST) {
                const auto& tile=o.self().tiles[cell/10][cell%10];
                if(tile.kind==T_PLANT && tile.what==WHEAT)balance[WHEAT]+=tile.yield_units;
            }
            if(task.input>=0){--balance[task.input];required[task.input]=std::max(required[task.input],-balance[task.input]);}
        }
    }
    return required;
}

template<int Mode> class Routes {
    std::array<std::array<int,100>,MAX_UNITS> routes_{};
    std::array<int,MAX_UNITS> sizes_{},cursor_{};
    std::array<bool,100> sites_{};
    int day_=-1,units_=0;
    bool active_=false;

    int cost(const agent::AgentObservation& o,const Context& c,int u,const std::array<int,100>& route,int size) const {
        int needs[N_ITEMS]{};
        for(int i=0;i<size;++i)for(int j=0;j<c.task_count[route[i]];++j) {
            int item=c.tasks[route[i]][j].input;if(item>=0)++needs[item];
        }
        if constexpr(Mode&1){auto required=required_inputs(o,c,route,size);std::copy_n(required.begin(),N_ITEMS,needs);}
        int x=o.self().pos_x[u],y=o.self().pos_y[u],time=0,pickups=0;
        if constexpr(Mode&4)for(int item=0;item<N_ITEMS;++item)
            time+=24*std::max(0,needs[item]-int(o.own.inv[u][item])-int(o.own.shed[item]));
        for(int item=0;item<N_ITEMS;++item)pickups+=needs[item]>o.own.inv[u][item];
        if(pickups){time+=shed_distance(x,y)+pickups;x=std::clamp(x,4,5);y=std::clamp(y,4,5);}
        for(int i=0;i<size;++i) {
            int cell=route[i];time+=distance(x,y,cell%10,cell/10)+c.task_count[cell];x=cell%10;y=cell/10;
        }
        return time+shed_distance(x,y)+1;
    }
    void assign(const agent::AgentObservation& o,const Context& c) {
        sizes_.fill(0);cursor_.fill(0);sites_=c.sites;units_=o.self().n_units;
        std::array<int,100> cells{};int count=0;
        for(int cell=0;cell<100;++cell)if(c.sites[cell])cells[count++]=cell;
        std::sort(cells.begin(),cells.begin()+count,[](int a,int b) {
            int da=shed_distance(a%10,a/10),db=shed_distance(b%10,b/10);return da!=db?da>db:a<b;
        });
        for(int i=0;i<count;++i) {
            int cell=cells[i],best=-1,position=-1,score=1000000;
            for(int u=0;u<units_;++u)for(int p=0;p<=sizes_[u];++p) {
                auto route=routes_[u];for(int j=sizes_[u];j>p;--j)route[j]=route[j-1];route[p]=cell;
                int value=cost(o,c,u,route,sizes_[u]+1);
                if(value<score){best=u;position=p;score=value;}
            }
            if(best<0)std::abort();
            for(int j=sizes_[best];j>position;--j)routes_[best][j]=routes_[best][j-1];
            routes_[best][position]=cell;++sizes_[best];
        }
    }
public:
    int active_turns=0;
    uint32_t active_days=0;
    void reset(){day_=-1;units_=0;active_=false;active_turns=0;active_days=0;sizes_.fill(0);cursor_.fill(0);sites_.fill(false);}
    void apply(const agent::AgentObservation& o,Action& action,const Context& c,int start_day) {
        if(o.day!=day_){day_=o.day;active_=false;}
        if(o.day<start_day || o.day>=29)return;
        if(!active_){if(o.hour<3)return;assign(o,c);active_=true;}
        if(units_!=o.self().n_units || sites_!=c.sites)assign(o,c);
        ++active_turns;active_days|=uint32_t{1}<<o.day;
        const auto& farm=o.self();
        int available[N_ITEMS],space=100-o.own.shed_total,seeds[N_CROPS];
        std::copy_n(o.own.shed,N_ITEMS,available);std::copy_n(o.own.seeds,N_CROPS,seeds);
        for(int u=0;u<farm.n_units;++u) {
            auto& a=action.units[u];a={};int needs[N_ITEMS]{};
            for(int i=0;i<sizes_[u];++i)for(int j=0;j<c.task_count[routes_[u][i]];++j) {
                int item=c.tasks[routes_[u][i]][j].input;if(item>=0)++needs[item];
            }
            std::array<int,N_ITEMS> pickup_need{};std::copy_n(needs,N_ITEMS,pickup_need.begin());
            if constexpr(Mode&2)pickup_need=required_inputs(o,c,routes_[u],sizes_[u],cursor_[u]);
            int target=-1;
            for(int offset=0;offset<sizes_[u];++offset) {
                int index=(cursor_[u]+offset)%sizes_[u],cell=routes_[u][index];
                if(c.task_count[cell]){target=cell;cursor_[u]=index;break;}
            }
            int x=farm.pos_x[u],y=farm.pos_y[u];bool shed=shed_distance(x,y)==0;
            if(shed) {
                int surplus=-1;
                for(int item=0;item<N_ITEMS;++item)
                    if(o.own.inv[u][item]>needs[item] && (item==WHEAT || item==FERTILIZER || is_animal(item))) {
                        if(surplus<0 || o.own.inv[u][item]-needs[item]>o.own.inv[u][surplus]-needs[surplus])surplus=item;
                    }
                if(surplus>=0 && space>0) {
                    int n=std::min(space,int(o.own.inv[u][surplus])-needs[surplus]);
                    a={OP_PLACE,uint8_t(surplus),n};available[surplus]+=n;space-=n;continue;
                }
                int input=-1;
                for(int item=0;item<N_ITEMS;++item)if(pickup_need[item]>o.own.inv[u][item] && available[item]>0) {
                    if(input<0 || pickup_need[item]-o.own.inv[u][item]>pickup_need[input]-o.own.inv[u][input])input=item;
                }
                if(input>=0) {
                    int n=std::min(available[input],pickup_need[input]-int(o.own.inv[u][input]));
                    a={OP_PICKUP,uint8_t(input),n};available[input]-=n;space+=n;continue;
                }
                int product=-1;
                for(int item=0;item<N_PRODUCTS;++item)if(o.own.inv[u][item]>needs[item])
                    if(product<0 || (o.own.inv[u][item]-needs[item])*o.market.prices[item]>
                        (o.own.inv[u][product]-needs[product])*o.market.prices[product])product=item;
                if(product>=0 && space>0) {
                    int n=std::min(space,int(o.own.inv[u][product])-needs[product]);
                    a={OP_PLACE,uint8_t(product),n};available[product]+=n;space-=n;continue;
                }
            }
            if(target<0) {
                bool cargo=false;for(int item=0;item<N_ITEMS;++item)cargo|=o.own.inv[u][item]>0;
                if(cargo)a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));
                continue;
            }
            int choice=-1;double value=-1;
            for(int j=0;j<c.task_count[target];++j) {
                const auto& task=c.tasks[target][j];
                if(task.input>=0 && o.own.inv[u][task.input]<=0)continue;
                if(task.action.op==OP_PLANT && seeds[task.action.arg]<=0)continue;
                if(o.hour+distance(x,y,target%10,target/10)>task.deadline)continue;
                if(task.value>value){value=task.value;choice=j;}
            }
            if(choice<0) {
                bool missing_input=false;
                for(int j=0;j<c.task_count[target];++j)missing_input|=c.tasks[target][j].input>=0 && o.own.inv[u][c.tasks[target][j].input]<=0;
                if(missing_input && !shed)a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));
                continue;
            }
            if(x!=target%10 || y!=target/10){a=toward(x,y,target%10,target/10);continue;}
            a=c.tasks[target][choice].action;
            if(a.op==OP_PLANT)--seeds[a.arg];
        }
    }
};

template<class Base,int StartDay,int Mode> class Agent:public Base {
    Routes<Mode> routes_;
public:
    void reset(const agent::AgentInit& init){Base::reset(init);routes_.reset();}
    int routed_turns()const{return routes_.active_turns;}
    uint32_t routed_days()const{return routes_.active_days;}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget&,Action& a) {
        Context c;Base::plan_units(o,a,c);routes_.apply(o,a,c,StartDay);Base::plan_market(o,a,c);
    }
};
}
