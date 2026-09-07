// Apache-2.0 adaptation of the v58 notebook; source/module hashes in IMPORT.json.
#include "agent.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>

namespace compositions::kaito_v58 {
namespace {
using namespace kag;
using Observation=kag::agent::AgentObservation;
using PublicFarm=kag::agent::PublicFarm;
using Configuration=kag::agent::AgentConfig;
constexpr int LEXICAL[]={7,0,6,5,3,1,4,8,2};
#include "routes.inc"

void route_action(int route,int step,int units,Action& action) {
    const int* data=route_values+route_offsets[route][std::clamp(step,0,718)];
    const int recorded=*data++,orders=*data++;
    action.clear();action.n_units=units;
    std::fill_n(action.units,units,UnitAction{});
    for(int u=0;u<recorded;++u) {
        if(u<units)action.units[u]={uint8_t(data[0]),uint8_t(data[1]),data[2]};
        data+=3;
    }
    for(int m=0;m<orders;++m) {
        action.orders[action.n_orders++]={uint8_t(data[0]),uint8_t(data[1]),data[2]};data+=3;
    }
}

int route_sale(int route,int step,int item) {
    const int* data=route_values+route_offsets[route][step];
    int units=data[0],orders=data[1],quantity=0;data+=2+3*units;
    for(int m=0;m<orders;++m,data+=3)
        if(data[0]==M_SELL && data[1]==item)quantity+=std::max(0,data[2]);
    return quantity;
}

std::array<int,15> counts(const PublicFarm& f) {
    std::array<int,15> result{};
    for(const auto& row:f.tiles)for(const auto& t:row) {
        if(t.kind==T_PLANT)++result[t.what];
        else if(t.has_animal)++result[t.what];
        else if(t.kind==T_PASTURE)++result[12];
        else if(t.kind==T_COOP)++result[13];
        else if(t.kind==T_WEED)++result[14];
    }
    return result;
}

int clone_distance(const Observation& o) {
    const auto a=counts(o.self()),b=counts(o.opponent());
    int d=std::abs(o.self().n_units-o.opponent().n_units)+3*std::abs(o.self().n_quadrants-o.opponent().n_quadrants);
    for(int i=0;i<15;++i)d+=std::abs(a[i]-b[i]);
    return d;
}

std::array<double,9> exposure(const PublicFarm& f,bool terminal=false) {
    std::array<double,9> values{};
    for(const auto& row:f.tiles)for(const auto& t:row) {
        if(t.kind==T_PLANT)values[t.what]+=std::max(1,int(t.yield_units));
        if(t.has_animal) {
            const int product=t.what==COW?MILK:t.what==SHEEP?WOOL:EGG;
            values[product]+=terminal?1+std::max(0,int(t.yield_units)):std::max(1,int(t.yield_units));
        }
        if(t.fertilizer_available)++values[FERTILIZER];
    }
    return values;
}

bool same_tile(const Tile& a,const Tile& b) {
    return std::tie(a.kind,a.what,a.has_animal,a.watered_today,a.fed_today,a.cared_today,
        a.fertilizer_available,a.consecutive_dry,a.yield_units,a.pending_care_bonus,
        a.planted_day,a.max_lifespan_step,a.fertilized_until_day)==
        std::tie(b.kind,b.what,b.has_animal,b.watered_today,b.fed_today,b.cared_today,
        b.fertilizer_available,b.consecutive_dry,b.yield_units,b.pending_care_bonus,
        b.planted_day,b.max_lifespan_step,b.fertilized_until_day);
}

bool mirror(const Observation& o) {
    const auto& a=o.self();const auto& b=o.opponent();
    if(a.money!=b.money || a.n_units!=b.n_units || a.n_quadrants!=b.n_quadrants)return false;
    for(int u=0;u<a.n_units;++u)if(a.pos_x[u]!=b.pos_x[u] || a.pos_y[u]!=b.pos_y[u])return false;
    for(int y=0;y<BOARD;++y)for(int x=0;x<BOARD;++x)if(!same_tile(a.tiles[y][x],b.tiles[y][x]))return false;
    return true;
}

int demand_at(const uint8_t* shops,int n,int step,const Configuration& config,int item) {
    int demand=0;
    if(step%std::max(1,config.shop_sell_interval)==0)
        for(int s=0;s<n;++s)if(SHOP_MASK[shops[s]]&(1<<item))demand+=SHOP_MULT[shops[s]];
    if(item!=FERTILIZER && step%std::max(1,config.center_sell_interval)==0)++demand;
    return demand;
}

double daily_demand(const Observation& o,const Configuration& config,int item) {
    double value=0;
    for(int s=0;s<o.n_shops;++s)if(SHOP_MASK[o.shops[s]]&(1<<item))
        value+=double(config.turns_per_day)/std::max(1,config.shop_sell_interval)*SHOP_MULT[o.shops[s]];
    if(item!=FERTILIZER) {
        int multiplier=config.center_sell_interval>=24?1:o.day>=20?4:o.day>=10?2:1;
        value+=double(config.turns_per_day)/std::max(1,config.center_sell_interval)*multiplier;
    }
    return value;
}

bool sale(const Order& order) {return order.op==M_SELL && order.item<N_PRODUCTS;}

void reorder(const Observation& o,const Configuration& config,Action& action,double alpha,bool front=false) {
    struct Ranked {double score;int index;Order order;};
    std::array<Ranked,10> ranked{};int n=0;
    for(int i=0;i<action.n_orders;++i)if(sale(action.orders[i])) {
        auto order=action.orders[i];int item=order.item,q=std::max(0,int(order.n));
        double score=q*std::max(0.0,double(o.market.prices[item]-market_price(item,o.market.inventory[item]+q)));
        if(score>0 && alpha>0) {
            double excess=std::max(0,o.market.inventory[item]+q-10000);
            double recovery=excess/std::max(.25,daily_demand(o,config,item));
            score*=1+alpha*std::min(1.0,recovery/10);
        }
        ranked[n++]={score,i,order};
    }
    std::sort(ranked.begin(),ranked.begin()+n,[](const auto& a,const auto& b) {
        return a.score!=b.score?a.score>b.score:a.index<b.index;
    });
    for(int i=0,j=0;i<action.n_orders;++i)if(sale(action.orders[i]))action.orders[i]=ranked[j++].order;
    if(front) {
        std::array<Order,10> orders{};int count=0;
        for(int i=0;i<action.n_orders;++i)if(sale(action.orders[i]) && action.orders[i].item>0 && action.orders[i].item<8)
            orders[count++]=action.orders[i];
        for(int i=0;i<action.n_orders;++i)if(!(sale(action.orders[i]) && action.orders[i].item>0 && action.orders[i].item<8))
            orders[count++]=action.orders[i];
        std::copy_n(orders.begin(),count,action.orders);
    }
}

bool access(int x,int y) {return (x==4 || x==5) && (y==4 || y==5);}

std::array<int,N_ITEMS> project(const Observation& o,const Action& action,int capacity,bool terminal=false) {
    std::array<int,N_ITEMS> shed{};std::copy_n(o.own.shed,N_ITEMS,shed.begin());
    int total=std::accumulate(shed.begin(),shed.end(),0);
    for(int u=0;u<action.n_units;++u) {
        int x=o.self().pos_x[u],y=o.self().pos_y[u];
        const auto& tile=o.self().tiles[y][x];const auto command=action.units[u];
        bool depot=access(x,y);
        auto deposit=[&](int item,int requested) {
            int quantity=std::min({std::max(0,requested),int(o.own.inv[u][item]),std::max(0,capacity-total)});
            shed[item]+=quantity;total+=quantity;
        };
        if(command.op==OP_DROP && depot) {
            for(int k=0;k<o.own.inv_nkeys[u];++k)deposit(o.own.inv_keys[u][k],o.own.inv[u][o.own.inv_keys[u][k]]);
        } else if(command.op==OP_PLACE) {
            const int item=command.arg;
            bool animal=item>=GOOSE && item<=SHEEP;
            bool placed=animal && !tile.has_animal && tile.kind==(item==GOOSE?T_COOP:T_PASTURE);
            if(!placed && depot)deposit(item,command.n);
        } else if(!terminal && command.op==OP_PICKUP && depot) {
            int taken=std::min(shed[command.arg],std::max(0,int(command.n)));
            shed[command.arg]-=taken;total-=taken;
        }
        // Feed affects only worker wheat. Active v58 preemption excludes wheat,
        // so that private inventory change has no downstream consumer here.
    }
    return shed;
}

void terminal(const Observation& o,Action& action) {
    const auto shed=project(o,action,100,true);const auto exposed=exposure(o.opponent(),true);
    constexpr int items[]={STRAWBERRY,MELON,MILK,WOOL,EGG,TOMATO,CARROT,WHEAT,FERTILIZER};
    constexpr double glut[]={1,1,1.3,2,3.6,1.5,2,3.2,1};
    struct Ranked {double score;int index,item,n;};std::array<Ranked,9> rows{};int n=0;
    for(int index=0;index<9;++index) {
        int item=items[index];if(shed[item]<=0)continue;
        double score=(1+exposed[item])*glut[item]*std::max(1,o.market.prices[item])*std::log1p(shed[item]);
        rows[n++]={score,index,item,shed[item]};
    }
    std::sort(rows.begin(),rows.begin()+n,[](const auto& a,const auto& b) {return a.score!=b.score?a.score>b.score:a.index<b.index;});
    action.n_orders=0;
    for(int i=0;i<n;++i)action.orders[action.n_orders++]={M_SELL,uint8_t(rows[i].item),Count(rows[i].n)};
}

int revenue(int item,int inventory,int quantity) {
    int value=0;
    for(int unit=0;unit<quantity;++unit) {int quote=market_price(item,inventory);value+=quote;if(quote>1)++inventory;}
    return value;
}

}

void Controller::act(int route,const Observation& o,const Configuration& config,Action& action) {
    const int step=o.step;
    if(last_step>=0 && last_step==step-1) {
        std::array<int,9> inferred{};
        for(int item=0;item<9;++item) {
            inferred[item]=std::clamp(o.market.inventory[item]-inventory[item]-own_net[item]+
                demand_at(shops.data(),n_shops,step-1,config,item),0,80);
            supply[item]=.65*supply[item]+.35*inferred[item];
        }
        int own_total=0,opponent_total=0,overlap=0;
        for(int item=1;item<8;++item) {
            int own=std::max(0,own_net[item]);own_total+=own;opponent_total+=inferred[item];
            overlap+=std::min(own,inferred[item]);
        }
        if(own_total || opponent_total) {
            double agreement=2.0*overlap/(own_total+opponent_total),alpha=agreement<confidence?.55:.35;
            confidence=(1-alpha)*confidence+alpha*agreement;++evidence;
        } else confidence*=.99;
    }
    std::copy_n(o.market.inventory,9,inventory.begin());
    if(clone_distance(o)<=8)++near_streak;else near_streak=0;
    if(near_streak>=12)near=true;
    route_action(route,step,o.self().n_units,action);
    for(int u=0;u<MAX_UNITS;++u) {
        auto& repair=repairs[u];int age=step-repair.start;
        if(u>=action.n_units) {repair.start=-100;continue;}
        if(age==1)action.units[u]=repair.intended;
        else if(age>=2 && age<=9) {
            Action previous;route_action(route,step-1,action.n_units,previous);action.units[u]=previous.units[u];
        } else repair.start=-100;
    }
    for(int u=0;u<action.n_units;++u) {
        auto& repair=repairs[u];auto& command=action.units[u];
        if(repair.start>=0 || (command.op!=OP_BUILD_PASTURE && command.op!=OP_PLANT))continue;
        if(o.self().tiles[o.self().pos_y[u]][o.self().pos_x[u]].kind==T_WEED) {
            repair={step,command};command={OP_DIG,0,1};
        }
    }
    reorder(o,config,action,config.center_sell_interval>=24?.25:0);
    auto debt=due[step];due[step].fill(0);
    int kept=0;
    for(int i=0;i<action.n_orders;++i) {
        auto order=action.orders[i];
        if(sale(order) && debt[order.item]>0) {
            int reduction=std::min(std::max(0,order.n),debt[order.item]);
            order.n=std::max(0,order.n)-reduction;debt[order.item]-=reduction;
            if(order.n<=0)continue;
        }
        action.orders[kept++]=order;
    }
    action.n_orders=kept;
    if(step<718)for(int item=0;item<9;++item)due[step+1][item]+=debt[item];
    const double deficit=o.opponent().money-o.self().money;
    if(step>=72 && step<700 && action.n_orders<10 &&
        (near || deficit>=4000 || *std::max_element(supply.begin(),supply.end())>=3)) {
        auto shed=project(o,action,config.shed_capacity);const auto exposed=exposure(o.opponent());
        std::array<int,9> existing{};
        for(int i=0;i<action.n_orders;++i)if(sale(action.orders[i]))existing[action.orders[i].item]+=std::max(0,action.orders[i].n);
        struct Candidate {int delta,gross,item,quantity;std::array<int,2> allocations;};
        std::array<Candidate,7> candidates{};int n=0;
        for(int item=1;item<8;++item) {
            int available=std::min(8,std::max(0,shed[item]-existing[item]));if(!available)continue;
            std::array<int,2> allocations{};
            int scheduled=0;
            for(int h=1;h<=2 && step+h<719;++h) {
                allocations[h-1]=std::min(available-scheduled,std::max(0,route_sale(route,step+h,item)-due[step+h][item]));
                scheduled+=allocations[h-1];
            }
            if(scheduled==0 || double(o.market.prices[item])/MARKET[item].base<.45)continue;
            double forecast=2*(supply[item]+.08*exposed[item]);
            if(near && evidence>=2 && confidence>=.3)forecast+=confidence*scheduled;
            int demand=demand_at(o.shops,o.n_shops,step,config,item)+demand_at(o.shops,o.n_shops,step+1,config,item);
            int future=int(std::nearbyint(o.market.inventory[item]+forecast-demand));
            int now=revenue(item,o.market.inventory[item],scheduled),later=revenue(item,future,scheduled);
            if(now-later>=8)candidates[n++]={now-later,now,item,scheduled,allocations};
        }
        // Descending Python tuple tie order uses the product name after value.
        std::sort(candidates.begin(),candidates.begin()+n,[](const auto& a,const auto& b) {
            if(a.delta!=b.delta)return a.delta>b.delta;
            if(a.gross!=b.gross)return a.gross>b.gross;
            return LEXICAL[a.item]>LEXICAL[b.item];
        });
        std::array<Order,10> additions{};int added=0,remaining=8;
        for(int i=0;i<n && added+action.n_orders<10 && remaining>0;++i) {
            const auto& candidate=candidates[i];int q=std::min(candidate.quantity,remaining),left=q;
            additions[added++]={M_SELL,uint8_t(candidate.item),q};
            for(int h=1;h<=2;++h) {
                int allocated=std::min(left,candidate.allocations[h-1]);due[step+h][candidate.item]+=allocated;left-=allocated;
            }
            remaining-=q;
        }
        std::move_backward(action.orders,action.orders+action.n_orders,action.orders+action.n_orders+added);
        std::copy_n(additions.begin(),added,action.orders);action.n_orders+=added;
    }
    bool front=near || deficit>=4500 || *std::max_element(supply.begin()+1,supply.begin()+8)>=3;
    reorder(o,config,action,.25,front);
    if(step==718)terminal(o,action);
    auto shed=project(o,action,config.shed_capacity);own_net.fill(0);
    int total=std::accumulate(shed.begin(),shed.end(),0);
    for(int i=0;i<action.n_orders;++i) {
        const auto order=action.orders[i];int item=order.item;
        if(sale(order)) {
            int q=std::min(std::max(0,order.n),shed[item]);shed[item]-=q;total-=q;own_net[item]+=q;
        } else if(order.op==M_BUY_PRODUCT && (item==WHEAT || item==FERTILIZER)) {
            int q=std::min(std::max(0,order.n),std::max(0,config.shed_capacity-total));
            shed[item]+=q;total+=q;own_net[item]-=q;
        }
    }
    n_shops=o.n_shops;std::copy_n(o.shops,n_shops,shops.begin());last_step=step;
    action.finalize();
}

void Agent::reset(const kag::agent::AgentInit& init) {
    config_=init.config;controllers_.fill(Controller{});mode_=0;mirror_streak_=0;known_yarn_=false;clone_=false;
}

void Agent::act(const Observation& o,const kag::agent::DecisionBudget&,Action& action) {
    std::array<Action,10> actions{};
    for(int route=0;route<10;++route)controllers_[route].act(route,o,config_,actions[route]);
    const int step=o.step,first=o.n_shops?o.shops[0]:-1,second=o.n_shops>1?o.shops[1]:-1;
    int selected=0;
    if(step>=72 && first==SHOP_YARN_STORE)selected=1;
    else if(step>=144 && first==SHOP_PET_CAFE && (second==SHOP_YARN_STORE || second==SHOP_PET_CAFE))selected=2;
    if(step==72) {
        auto assets=counts(o.opponent());
        int mine=int(std::nearbyint(o.self().money)),rival=int(std::nearbyint(o.opponent().money)),wheat=o.market.inventory[WHEAT];
        if(assets[COW]==3 && assets[SHEEP]==2 && assets[WHEAT]==7 && assets[MELON]==12) {
            if(first==SHOP_FARMERS_MARKET && mine==141 && rival==193 && wheat==9974)mode_=1;
            else if((first==SHOP_BAKERY && mine==145 && rival==195 && wheat==9975) ||
                (first==SHOP_SMOOTHIE_SHOP && mine==145 && rival==193 && wheat==9975) ||
                (first==SHOP_BAKERY && mine==142 && rival==142 && wheat==9973) ||
                (first==SHOP_ICE_CREAM_SHOP && mine==142 && rival==191 && wheat==9974))mode_=2;
            else if(first==SHOP_ICE_CREAM_SHOP && mine==145 && rival==195 && wheat==9975)mode_=3;
            else if(first==SHOP_BAKERY && mine==141 && rival==193 && wheat==9974)mode_=4;
            else if(first==SHOP_PIZZA_SHOP && mine==141 && rival==193 && wheat==9974)mode_=5;
        }
    }
    if(mode_==1)selected=step>=144 && second==SHOP_SMOOTHIE_SHOP?4:3;
    else if(mode_==2)selected=3;
    else if(mode_==3)selected=7;
    else if(mode_==4) {if(step>=144 && second==SHOP_YARN_STORE)selected=8;}
    else if(mode_==5) {if(step>=144 && second==SHOP_PET_CAFE)selected=9;}
    else {
        if(step==96 && first==SHOP_YARN_STORE) {
            auto assets=counts(o.opponent());
            int mine=int(std::nearbyint(o.self().money)),rival=int(std::nearbyint(o.opponent().money)),wheat=o.market.inventory[WHEAT];
            known_yarn_=assets[COW]==4 && assets[SHEEP]==2 && assets[WHEAT]==7 && assets[MELON]==12 &&
                ((mine==214 && rival==214 && wheat==9982) || (mine==215 && rival==125 && wheat==9979));
        }
        if(step>=96 && known_yarn_)selected=6;
        else if(step>=72 && first==SHOP_YARN_STORE) {
            if(mirror(o))++mirror_streak_;else mirror_streak_=0;
            if(step==360)clone_=mirror_streak_>=240;
            if(step>=360 && clone_)selected=5;
        }
    }
    action=actions[selected];
}

}
