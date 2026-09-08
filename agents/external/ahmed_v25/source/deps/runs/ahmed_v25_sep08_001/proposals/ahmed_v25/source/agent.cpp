#include "agent.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace catalog_ahmed_v25_compositions::ahmed_v25 {
using namespace kag;
namespace {
#include "data.inc"
using Obs=kag::agent::AgentObservation;
using Stock=std::array<int,kag::N_ITEMS>;
// Python sorts product strings alphabetically when prices tie.
constexpr int alpha[9]={7,0,6,5,3,1,4,8,2};
constexpr int seed_price[5]={10,20,50,100,80};
bool adjacent(const Obs& o,int u){const auto& f=o.self();return (f.pos_x[u]==4 || f.pos_x[u]==5) && (f.pos_y[u]==4 || f.pos_y[u]==5);}
bool move(int op){return op>=OP_NORTH && op<=OP_WEST;}
int held(const Obs& o,int item){int n=0;for(int u=0;u<o.self().n_units;++u)n+=o.own.inv[u][item];return n;}
void tape(int route,int step,Action& a){
    a.clear();if(step<0 || step>=719){a.n_units=1;return;}
    int at=offsets[route][step];a.n_units=values[at++];a.n_orders=values[at++];
    for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(values[at]),uint8_t(values[at+1]),values[at+2]};at+=3;}
    for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(values[at]),uint8_t(values[at+1]),values[at+2]};at+=3;}
}
bool noop(const Obs& o,int u,const UnitAction& a){
    const auto& f=o.self();const int x=f.pos_x[u],y=f.pos_y[u];const auto& t=f.tiles[y][x];
    if(move(a.op))return (a.op==OP_NORTH && y==0)||(a.op==OP_SOUTH && y==9)||(a.op==OP_EAST && x==9)||(a.op==OP_WEST && x==0);
    if(a.op==OP_PASS)return true;
    if(a.op==OP_DROP)return !adjacent(o,u) || !o.own.inv_nkeys[u];
    if(a.op==OP_PICKUP)return !adjacent(o,u);
    if(a.op==OP_PLACE){
        if(a.arg>=GOOSE && ((a.arg==GOOSE && t.kind==T_COOP)||(a.arg!=GOOSE && t.kind==T_PASTURE)) && !t.has_animal)
            return o.own.inv[u][a.arg]<=0;
        return !adjacent(o,u) || o.own.inv[u][a.arg]<=0;
    }
    if(t.kind==T_LOCKED)return true;
    switch(a.op){
    case OP_PLANT:return t.kind!=T_EMPTY || a.arg>=N_CROPS || o.own.seeds[a.arg]<=0;
    case OP_WATER:return t.kind!=T_PLANT || t.watered_today;
    case OP_HARVEST:return t.kind==T_EMPTY || t.yield_units<=0;
    case OP_FERTILIZE:return t.kind!=T_PLANT || o.own.inv[u][FERTILIZER]<=0;
    case OP_DIG:return t.kind==T_EMPTY || t.has_animal;
    case OP_BUILD_COOP:case OP_BUILD_PASTURE:return t.kind!=T_EMPTY;
    case OP_FEED:return !t.has_animal || t.fed_today || o.own.inv[u][WHEAT]<=0;
    case OP_COLLECT_FERTILIZER:return !t.has_animal || !t.fertilizer_available;
    case OP_CARE:return !t.has_animal || t.cared_today;
    default:return true;
    }
}
Stock projected(const Obs& o,const Action& a){
    Stock s{};for(int i=0;i<N_ITEMS;++i)s[i]=o.own.shed[i];int total=o.own.shed_total;
    for(int u=0;u<a.n_units;++u){
        if(!adjacent(o,u))continue;
        const auto& v=a.units[u];
        if(v.op==OP_PICKUP){int n=std::min(s[v.arg],std::max(0,int(v.n)));s[v.arg]-=n;total-=n;}
        else if(v.op==OP_DROP){
            for(int k=0;k<o.own.inv_nkeys[u];++k){int i=o.own.inv_keys[u][k];int n=std::min(int(o.own.inv[u][i]),std::max(0,100-total));s[i]+=n;total+=n;}
        }else if(v.op==OP_PLACE && v.arg<GOOSE){int n=std::min({std::max(0,int(v.n)),int(o.own.inv[u][v.arg]),std::max(0,100-total)});s[v.arg]+=n;total+=n;}
    }
    return s;
}
bool add_sell(Action& a,int item,int n,bool merge=true){
    if(merge)for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==M_SELL && a.orders[i].item==item){a.orders[i].n+=n;return true;}
    if(a.n_orders>=10)return false;
    a.orders[a.n_orders++]={M_SELL,uint8_t(item),n};return true;
}
Stock sales(const Action& a){Stock s{};for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==M_SELL)s[a.orders[i].item]+=std::max(0,int(a.orders[i].n));return s;}
}
void Agent::clear_state(){
    for(auto& q:pending_)q.clear();route_=branch_=0;last_step_=due_=-1;
    first_=production_=crop_=false;suppression_.fill(0);
}
void Agent::act(const Obs& o,const kag::agent::DecisionBudget&,Action& a){
    if(o.step==0 || o.step<=last_step_)clear_state();last_step_=o.step;
    if(!production_ && o.step>=144){
        route_=0;for(int i=0;i<o.n_shops;++i)if(o.shops[i]==SHOP_YARN_STORE)route_=1;
        production_=true;
    }
    if(!crop_ && o.step>=648){route_=o.market.inventory[EGG]<=9888?2:3;crop_=true;}
    tape(route_,o.step,a);for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};a.n_units=o.self().n_units;
    Action next;tape(route_,o.step+1,next);
    for(int u=0;u<a.n_units;++u){
        int x=o.self().pos_x[u],y=o.self().pos_y[u];const auto& t=o.self().tiles[y][x];auto act=a.units[u];auto& q=pending_[u];
        if(q.size && (q.front().x!=x || q.front().y!=y))q.clear();
        bool idle=noop(o,u,act);int next_op=u<next.n_units?next.units[u].op:OP_PASS;
        if((act.op==OP_PLANT || act.op==OP_BUILD_COOP || act.op==OP_BUILD_PASTURE) && t.kind==T_WEED){q.push({act,int8_t(x),int8_t(y)});act={OP_DIG,0,1};}
        else if(q.size && idle){
            if(q.front().action.op==OP_PLANT && move(next_op))q.clear();
            else{auto replay=q.pop();if(act.op!=OP_PASS && !move(act.op))q.push({act,int8_t(x),int8_t(y)});act=replay.action;}
        }else if(t.kind==T_WEED && idle)act={OP_DIG,0,1};
        a.units[u]=act;
    }
    if(due_==o.step)for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==M_SELL && m.item<9){int removed=std::min(std::max(0,int(m.n)),suppression_[m.item]);m.n-=removed;suppression_[m.item]-=removed;}}
    const Stock proj=projected(o,a);Stock available=proj;due_=-1;suppression_.fill(0);
    if(o.step+1<=718 && (o.step+1)%72 && o.step%4){
        auto planned=sales(next);bool already[9]{};for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL && a.orders[j].item<9)already[a.orders[j].item]=true;
        for(int i=1;i<8;++i){if(already[i] || planned[i]<=0 || o.market.prices[i]<2)continue;int qty=std::min(available[i],planned[i]);if(qty<=0)continue;
            if(!add_sell(a,i,qty,false))break;available[i]-=qty;suppression_[i]+=qty;due_=o.step+1;
        }
    }
    if(o.step>=718){
        for(int u=0;u<a.n_units;++u){
            bool cargo=false;for(int i=0;i<N_ITEMS;++i)cargo|=o.own.inv[u][i]>0;
            a.units[u]={uint8_t(adjacent(o,u) && cargo?OP_DROP:OP_PASS),0,1};
        }
        a.n_orders=0;const auto terminal=projected(o,a);
        for(int i=0;i<9;++i)if(terminal[i]>0)a.orders[a.n_orders++]={M_SELL,uint8_t(i),terminal[i]};
        std::stable_sort(a.orders,a.orders+a.n_orders,[&](const auto& x,const auto& y){
            return int64_t(o.market.prices[x.item])*x.n>int64_t(o.market.prices[y.item])*y.n;
        });
    }
    for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==M_NONE || (m.op>M_BUY_LAND && m.n<=0))m={M_SELL,WHEAT,0};}
    a.finalize();
}
}
