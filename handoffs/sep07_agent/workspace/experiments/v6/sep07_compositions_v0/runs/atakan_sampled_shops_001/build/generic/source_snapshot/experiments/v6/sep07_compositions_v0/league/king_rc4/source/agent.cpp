#include "agent.hpp"
#include <algorithm>
#include <cmath>
#include <string_view>

namespace compositions::king_rc4 {
namespace {
using namespace kag;
using Obs = agent::AgentObservation;
struct MarketExample { int feature[43]; int count; Order orders[10]; };
#include "data.inc"
constexpr std::string_view names[]={"WHEAT","CARROT","TOMATO","STRAWBERRY","MELON","EGG","MILK","WOOL","FERTILIZER"};
constexpr int seed_cost[]={10,20,50,100,80};
constexpr int animal_cost[]={300,400,500};
struct List {
    Order data[20]{}; int n=0;
    List() = default;
    explicit List(const Action& a) { n=a.n_orders; std::copy_n(a.orders,n,data); }
    void add(Order o) { if(n>=20)std::abort(); data[n++]=o; }
    void save(Action& a) const { a.n_orders=std::min(10,n); std::copy_n(data,a.n_orders,a.orders); }
    bool sells(int item) const { for(int i=0;i<n;++i)if(data[i].op==M_SELL && data[i].item==item)return true; return false; }
    void extra(int item,int q) { if(q>0 && n<10 && !sells(item))add({M_SELL,uint8_t(item),q}); }
    void merge() {
        List out; int qty[N_ITEMS]{}, order[N_ITEMS]{}, count=0;
        for(int i=0;i<n;++i)if(data[i].op==M_SELL) {
            int it=data[i].item; bool seen=false;
            for(int j=0;j<count;++j)seen|=order[j]==it;
            if(!seen)order[count++]=it;
            qty[it]+=std::max(0,data[i].n);
        }
        for(int j=0;j<count;++j)if(qty[order[j]]>0)out.add({M_SELL,uint8_t(order[j]),qty[order[j]]});
        for(int i=0;i<n;++i)if(data[i].op!=M_SELL)out.add(data[i]);
        *this=out; n=std::min(n,10);
    }
};
struct Tables {
    Action actions[3738]{};
    Tables() {
        for(int i=0;i<3738;++i) {
            auto& a=actions[i]; int cursor=action_offset[i];
            a.n_units=action_data[cursor++]; a.n_orders=action_data[cursor++];
            if(a.n_units>MAX_UNITS || a.n_orders>10)std::abort();
            for(int u=0;u<a.n_units;++u) {a.units[u]={uint8_t(action_data[cursor]),uint8_t(action_data[cursor+1]),action_data[cursor+2]};cursor+=3;}
            for(int j=0;j<a.n_orders;++j) {a.orders[j]={uint8_t(action_data[cursor]),uint8_t(action_data[cursor+1]),action_data[cursor+2]};cursor+=3;}
            a.finalize();
        }
    }
};
const Tables& tables() { static const Tables data; return data; }
int category(int shop) {
    if(shop==SHOP_ICE_CREAM_SHOP || shop==SHOP_PIZZA_SHOP || shop==SHOP_SMOOTHIE_SHOP)return 1;
    if(shop==SHOP_PET_CAFE)return 2;
    if(shop==SHOP_YARN_STORE)return 3;
    return 0;
}
bool has_shop(const Obs& o,int shop) {return std::find(o.shops,o.shops+o.n_shops,shop)!=o.shops+o.n_shops;}
int index(const Obs& o,int step,bool s1_bal) {
    int phase=std::min(9,step/72), family=category(o.n_shops?o.shops[0]:-1);
    if(phase==1 && s1_bal)family=0;
    if(phase>=2 && family!=3 && o.n_shops>=2 && o.shops[1]==SHOP_YARN_STORE)family=4;
    if(phase>=3 && family<3 && o.n_shops>=3 && o.shops[2]==SHOP_YARN_STORE)family=5;
    return block_start[block_map[phase][family]] + step%72;
}
void normalize(const Obs& o,Action& a) {
    const int old=a.n_units; a.n_units=o.self().n_units;
    for(int u=old;u<a.n_units;++u)a.units[u]={};
}
std::array<int,43> features(const Obs& o) {
    std::array<int,43> f{};
    for(int i=0;i<9;++i)f[i]=o.market.inventory[i];
    for(int i=0;i<12;++i)f[9+i]=o.own.shed[i];
    for(int i=0;i<5;++i)f[21+i]=o.own.seeds[i];
    f[26]=int(o.self().money);
    for(const auto& row:o.self().tiles)for(const auto& t:row) {
        if(t.has_animal)++f[t.what==COW?27:t.what==SHEEP?28:29];
        if(t.kind==T_PLANT)++f[30+t.what];
    }
    constexpr int shop_order[]={SHOP_BAKERY,SHOP_PIZZA_SHOP,SHOP_BRUNCH_SPOT,SHOP_YARN_STORE,SHOP_ICE_CREAM_SHOP,SHOP_PET_CAFE,SHOP_SMOOTHIE_SHOP,SHOP_FARMERS_MARKET};
    for(int i=0;i<8;++i)f[35+i]=std::count(o.shops,o.shops+o.n_shops,shop_order[i]);
    return f;
}
double distance(const std::array<int,43>& a,const int* b) {
    constexpr double scale[]={400,450,200,100,300,332,122,105,200};
    double d=0,v=0;
    for(int i=0;i<9;++i)d+=1.5*std::abs(a[i]-b[i])/scale[i];
    for(int i=9;i<21;++i)v+=.35*std::abs(a[i]-b[i])/10.; d+=v;v=0;
    for(int i=21;i<26;++i)v+=.25*std::abs(a[i]-b[i])/10.;d+=v;v=0;
    d+=.15*std::abs(a[26]-b[26])/2000.;
    for(int i=27;i<30;++i)v+=.7*std::abs(a[i]-b[i])/4.;d+=v;v=0;
    for(int i=30;i<35;++i)v+=.4*std::abs(a[i]-b[i])/10.;d+=v;v=0;
    for(int i=35;i<43;++i)v+=2.*std::abs(a[i]-b[i]);return d+v;
}
bool signature(const agent::PublicFarm& a,const agent::PublicFarm& b) {
    if(a.n_units!=b.n_units || a.n_quadrants!=b.n_quadrants)return false;
    for(int u=0;u<a.n_units;++u)if(a.pos_x[u]!=b.pos_x[u] || a.pos_y[u]!=b.pos_y[u])return false;
    for(int y=0;y<BOARD;++y)for(int x=0;x<BOARD;++x) {
        const auto& t=a.tiles[y][x];const auto& s=b.tiles[y][x];
        if(t.has_animal!=s.has_animal)return false;
        if(t.has_animal) {if(t.what!=s.what)return false;}
        else if(t.kind!=s.kind || (t.kind==T_PLANT && t.what!=s.what))return false;
    }
    return true;
}
std::array<int,10> production_counts(const agent::PublicFarm& f) {
    std::array<int,10> c{};
    for(const auto& row:f.tiles)for(const auto& t:row) {
        if(t.has_animal)++c[t.what==COW?5:t.what==SHEEP?6:7];
        else if(t.kind==T_PLANT)++c[t.what];
        else if(t.kind==T_PASTURE)++c[8];
        else if(t.kind==T_COOP)++c[9];
    }
    return c;
}
int fib(int n) { int a=1,b=1;for(int i=0;i<n;++i){int c=a+b;a=b;b=c;}return a; }
void terminal(const Obs& o,Action& a) {
    if(o.step<708)return;
    constexpr int centers[4][2]={{4,4},{5,4},{4,5},{5,5}};
    const auto& f=o.self();
    for(int u=0;u<a.n_units;++u) {
        int carried=0;for(int i=0;i<9;++i)carried+=o.own.inv[u][i];if(!carried)continue;
        int x=f.pos_x[u],y=f.pos_y[u],best=0,dist=100;
        for(int k=0;k<4;++k) {int d=std::abs(x-centers[k][0])+std::abs(y-centers[k][1]);if(d<dist){best=k;dist=d;}}
        if(o.step<718-dist)continue;
        int op=dist==0?OP_DROP:x<centers[best][0]?OP_EAST:x>centers[best][0]?OP_WEST:y<centers[best][1]?OP_SOUTH:OP_NORTH;
        a.units[u]={uint8_t(op),0,1};
    }
    if(o.step<709)return;
    int predicted[9]{},covered[9]{};for(int i=0;i<9;++i)predicted[i]=o.own.shed[i];
    for(int u=0;u<a.n_units;++u)if(a.units[u].op==OP_DROP && is_shed_adjacent(f.pos_x[u],f.pos_y[u],BOARD))
        for(int i=0;i<9;++i)predicted[i]+=o.own.inv[u][i];
    List m(a),extra;
    for(int j=0;j<m.n;++j)if(m.data[j].op==M_SELL && m.data[j].item<9)covered[m.data[j].item]+=std::max(0,m.data[j].n);
    for(int i=0;i<9;++i)if(predicted[i]>covered[i])extra.add({M_SELL,uint8_t(i),predicted[i]-covered[i]});
    std::sort(extra.data,extra.data+extra.n,[&](auto x,auto y){return o.market.prices[x.item]!=o.market.prices[y.item]?o.market.prices[x.item]>o.market.prices[y.item]:names[x.item]>names[y.item];});
    for(int i=0;i<extra.n && m.n<10;++i)m.add(extra.data[i]); m.save(a);
}
}

void Agent::reset(const kag::agent::AgentInit& init) {
    tail_.reset(init);s1_bal_=low_=highcap_=yarn_second_=smartfarm_=false;queue_day_=-1;
    pending_.fill(false);delayed_.fill({});throttle_.fill(-999);due_.fill(0);
}

void Agent::base(const Obs& o,Action& a) {
    if(o.step==72 && o.n_shops && o.shops[0]==SHOP_YARN_STORE && o.opponent().money<=180)s1_bal_=true;
    int i=index(o,o.step,s1_bal_);a=tables().actions[i];
    int first=example_range[i][0],end=example_range[i][1];
    if(first<end) {
        auto f=features(o);int best=first;double d=distance(f,market_examples[first].feature);
        for(int k=first+1;k<end;++k){double v=distance(f,market_examples[k].feature);if(v<d){best=k;d=v;}}
        const auto& m=market_examples[best];a.n_orders=m.count;std::copy_n(m.orders,m.count,a.orders);
    }
    normalize(o,a);
    if(o.step==0) {a.n_orders=1;a.orders[0]={M_BUY_PRODUCT,WHEAT,6};}
    if(o.step==1) {
        List rest, m;
        for(int j=0;j<a.n_orders;++j)if(!(a.orders[j].op==M_BUY_PRODUCT && a.orders[j].item==WHEAT))rest.add(a.orders[j]);
        m.add({M_SELL,WHEAT,1});
        if(o.self().money+std::max(0,o.market.prices[WHEAT])+1e-9<2842) {
            auto rank=[](Order q){return q.op==M_HIRE || q.op==M_BUY_ANIMAL || q.op==M_BUY_LAND?0:q.op==M_BUY_SEED?1:2;};
            std::stable_sort(rest.data,rest.data+rest.n,[&](auto x,auto y){return rank(x)<rank(y);});
        }
        for(int j=0;j<rest.n;++j)m.add(rest.data[j]);m.save(a);
    }
    terminal(o,a);
}

void Agent::generic(const Obs& o,Action& a) {
    base(o,a);
    if(queue_day_!=o.step/24) {pending_.fill(false);queue_day_=o.step/24;}
    for(int u=0;u<a.n_units;++u) {
        const auto original=a.units[u];
        if(pending_[u]) {
            a.units[u]=delayed_[u];pending_[u]=original.op!=OP_PASS;
            if(pending_[u])delayed_[u]=original;
        } else if(o.self().tiles[o.self().pos_y[u]][o.self().pos_x[u]].kind==T_WEED &&
            (original.op==OP_BUILD_PASTURE || original.op==OP_BUILD_COOP ||
             (original.op==OP_PLANT && o.own.seeds[original.arg]>0))) {
            a.units[u]={OP_DIG,0,1};pending_[u]=true;delayed_[u]=original;
        }
    }
    List m(a);
    if(o.step==0 || o.step==1) {
        List out;out.add({uint8_t(o.step==0?M_BUY_PRODUCT:M_SELL),WHEAT,o.step==0?30:25});
        for(int j=0;j<m.n;++j)if(!(m.data[j].item==WHEAT && (m.data[j].op==M_BUY_PRODUCT || m.data[j].op==M_SELL)))out.add(m.data[j]);
        m=out;
        if(o.step==1)low_=std::max(0,9969-o.market.inventory[WHEAT])<=5;
    }
    m.merge();
    const bool same=signature(o.self(),o.opponent());
    if(o.step>=2 && low_ && same) {
        double cash=o.opponent().money;List out;
        for(int j=0;j<m.n;++j) {
            auto q=m.data[j];
            if(q.op==M_SELL)cash+=std::max(0,q.n)*double(o.market.prices[q.item]);
            if(q.op==M_BUY_PRODUCT && q.item==WHEAT) {
                const double price=std::max(1,o.market.prices[WHEAT]);
                q.n=std::min(std::max(0,q.n),std::max(0,int(std::floor((cash-1)/price))));
                if(q.n<=0)continue;cash-=q.n*price;
            }
            out.add(q);
        }
        m=out;
    }
    if(o.step>=600 && same && std::abs(o.self().money-o.opponent().money)<=10)
        std::stable_sort(m.data,m.data+m.n,[](auto x,auto y){
            if((x.op==M_SELL)!=(y.op==M_SELL))return x.op==M_SELL;
            if(x.op!=M_SELL)return false;
            return x.n!=y.n?x.n>y.n:names[x.item]<names[y.item];
        });
    if(o.step>=708)std::stable_sort(m.data,m.data+m.n,[](auto x,auto y){
        if((x.op==M_SELL)!=(y.op==M_SELL))return x.op==M_SELL;
        return x.op==M_SELL && x.n>y.n;
    });
    m.save(a);
}

void Agent::sales(const Obs& o,Action& a,bool throttled) {
    if(o.step<240)return;
    List m(a);
    bool dairy=has_shop(o,SHOP_SMOOTHIE_SHOP)||has_shop(o,SHOP_PIZZA_SHOP)||has_shop(o,SHOP_ICE_CREAM_SHOP);
    bool berry=has_shop(o,SHOP_SMOOTHIE_SHOP)||has_shop(o,SHOP_BRUNCH_SPOT)||has_shop(o,SHOP_ICE_CREAM_SHOP)||has_shop(o,SHOP_FARMERS_MARKET);
    bool carrot=has_shop(o,SHOP_PET_CAFE)||has_shop(o,SHOP_FARMERS_MARKET);
    auto add=[&](int item,int q) {
        if(throttled && o.step-throttle_[item]<4)return;
        int before=m.n;m.extra(item,q);if(throttled && m.n>before)throttle_[item]=o.step;
    };
    const auto* shed=o.own.shed;const auto* prices=o.market.prices;
    if(dairy && shed[MILK]>=4 && prices[MILK]>=80)add(MILK,std::min(4,int(shed[MILK])));
    if(o.step%4==1) {
        bool wool_price=throttled?(prices[WOOL]>=150 || o.own.shed_total>=85):prices[WOOL]>=35;
        if(has_shop(o,SHOP_YARN_STORE) && shed[WOOL]>=2 && wool_price)add(WOOL,std::min(3,int(shed[WOOL])));
        if(berry && shed[STRAWBERRY]>=3 && prices[STRAWBERRY]>=85)add(STRAWBERRY,std::min(3,int(shed[STRAWBERRY])));
        if(carrot && shed[CARROT]>=2 && prices[CARROT]>=30)add(CARROT,std::min(3,int(shed[CARROT])));
    }
    int st=0,mel=0;
    for(const auto& row:o.opponent().tiles)for(const auto& t:row)if(t.kind==T_PLANT) {
        int age=o.day-t.planted_day;
        if(t.what==STRAWBERRY && (age>=8 || t.yield_units>0))++st;
        else if(t.what==MELON && (age>=9 || t.yield_units>0))++mel;
    }
    if(st>=4 && shed[STRAWBERRY]>=3 && prices[STRAWBERRY]>=100)add(STRAWBERRY,std::min(4,int(shed[STRAWBERRY])));
    if(mel>=2 && shed[MELON]>=2 && prices[MELON]>=120)add(MELON,std::min(4,int(shed[MELON])));
    m.merge();m.save(a);
}

void Agent::tail(const Obs& o,Action& a) {
    tail_.act(o,{},a);List m;
    for(int j=0;j<a.n_orders;++j) {
        auto q=a.orders[j];
        if(q.op==M_SELL && due_[q.item]>0) {
            int cut=std::min(q.n,due_[q.item]);q.n-=cut;due_[q.item]-=cut;if(q.n<=0)continue;
        }
        m.add(q);
    }
    due_.fill(0);
    // The donor's immutable tail has 720 rows, including the unused final row.
    if(o.step+1<720 && m.n<10) {
        int next[N_PRODUCTS]{},current[N_PRODUCTS]{};
        const auto& raw=tail_.planned_action(o.step+1);
        for(int j=0;j<raw.n_orders;++j)if(raw.orders[j].op==M_SELL)next[raw.orders[j].item]+=raw.orders[j].n;
        for(int j=0;j<m.n;++j)if(m.data[j].op==M_SELL)current[m.data[j].item]+=m.data[j].n;
        int best=-1,qty=0,value=0;
        for(int i=0;i<N_PRODUCTS;++i) {
            int q=std::min({2,next[i],std::max(0,int(o.own.shed[i])-current[i])});
            int v=o.market.prices[i]*q;
            if(q>0 && o.market.prices[i]>1 && (v>value || (v==value && (best<0 || names[i]>names[best])))) {best=i;qty=q;value=v;}
        }
        if(best>=0) {m.add({M_SELL,uint8_t(best),qty});due_[best]=qty;}
    }
    m.save(a);
}

void Agent::reserve(const Obs& o,Action& a) {
    if(highcap_ || o.step%24<12)return;
    int next=(o.step/24+1)*24;if(next>718)return;
    const auto& raw=tables().actions[index(o,next,s1_bal_)];int hires=0,amount=0;
    for(int j=0;j<raw.n_orders;++j) {if(raw.orders[j].op==M_SELL)break;if(raw.orders[j].op==M_HIRE)amount+=fib(hires++);}
    if(amount<=0)return;
    for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL)return;
    double cash=o.self().money;List m;
    for(int j=0;j<a.n_orders;++j) {
        auto q=a.orders[j];
        if(q.op==M_BUY_SEED || q.op==M_BUY_PRODUCT) {
            double unit=q.op==M_BUY_SEED?seed_cost[q.item]:o.market.prices[q.item];
            if(unit>0 && q.n>0) {
                q.n=std::min(q.n,std::max(0,int(std::floor((cash-amount)/unit))));
                if(q.n<=0)continue;cash-=q.n*unit;
            }
        }
        m.add(q);
    }
    m.save(a);
}

void Agent::rc3(const Obs& o,Action& a) {
    if(highcap_)return;
    if(o.step==241 && has_shop(o,SHOP_YARN_STORE)) {
        int at=-1;
        for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_BUY_ANIMAL && a.orders[j].item==SHEEP){at=j;break;}
        if(at>=0) {
            int picks=0,places=0;
            const int end=std::min({719,(o.step/72+1)*72,o.step+25});
            for(int s=o.step+1;s<end;++s) {
                const auto& raw=tables().actions[index(o,s,false)];
                for(int u=0;u<raw.n_units;++u)if(raw.units[u].arg==SHEEP) {
                    if(raw.units[u].op==OP_PICKUP)picks+=raw.units[u].n;
                    else if(raw.units[u].op==OP_PLACE)++places;
                }
            }
            int extra=std::min(1,std::max(0,std::min(picks,places)-int(o.own.shed[SHEEP])-a.orders[at].n));
            if(extra>0) {
                int cost=0,hires=o.self().hires_today;
                for(int j=0;j<a.n_orders;++j) {
                    auto q=a.orders[j];
                    if(q.op==M_HIRE)cost+=fib(hires++);
                    else if(q.op==M_BUY_ANIMAL)cost+=animal_cost[q.item-GOOSE]*std::max(0,q.n);
                    else if(q.op==M_BUY_SEED)cost+=seed_cost[q.item]*std::max(0,q.n);
                    else if(q.op==M_BUY_LAND) {int n=o.self().n_quadrants;cost+=n>=1 && n<=3?1000*(1<<(n-1)):0;}
                }
                if(o.self().money-cost>=500*extra+100)a.orders[at].n+=extra;
            }
        }
    }
    if(o.step==385 && o.own.shed[FERTILIZER]>=12) {
        bool active=o.opponent().money>0 || o.opponent().n_units>1;
        for(const auto& row:o.opponent().tiles)for(const auto& t:row)active|=t.kind==T_PLANT || t.has_animal;
        if(active) {
            List m;bool changed=false;
            for(int j=0;j<a.n_orders;++j) {
                auto q=a.orders[j];
                if(!changed && q.op==M_BUY_PRODUCT && q.item==FERTILIZER && q.n>0) {changed=true;if(--q.n==0)continue;}
                m.add(q);
            }
            m.save(a);
        }
    }
}

void Agent::act(const Obs& o,const kag::agent::DecisionBudget&,Action& a) {
    if(o.step==0)reset({{},o.player});
    a.clear();a.n_units=o.self().n_units;
    if(o.step>=719 || o.step<0) {a.finalize();return;}
    if(o.step==72)highcap_=o.opponent().money<=180 && (!o.n_shops || o.shops[0]!=SHOP_YARN_STORE);
    if(o.step==144 && !highcap_ && o.n_shops>=2 && o.shops[1]==SHOP_YARN_STORE && o.shops[0]!=SHOP_YARN_STORE)yarn_second_=true;
    if(highcap_ && o.step>=72)tail(o,a);
    else {
        generic(o,a);
        if(o.step>=240) {
            auto own=production_counts(o.self()),opp=production_counts(o.opponent());int d=0;
            for(int i=0;i<10;++i)d+=std::abs(own[i]-opp[i]);
            if(yarn_second_)sales(o,a,false);
            else if(!(low_ && d<=3))sales(o,a,!(opp[5]+opp[6]>=own[5]+own[6]+2 || d>=8));
        }
        if(!(yarn_second_ && o.step>=240))reserve(o,a);
    }
    rc3(o,a);
    if(o.step==2 && o.opponent().money==66 && o.opponent().pos_x[0]==4 && o.opponent().pos_y[0]==3 && o.opponent().n_units==6 && o.opponent().n_quadrants==1)smartfarm_=true;
    if(smartfarm_ && o.step>=577)std::stable_sort(a.orders,a.orders+a.n_orders,[](auto x,auto y){return (x.op==M_HIRE)<(y.op==M_HIRE);});
    normalize(o,a);a.finalize();
}
}
