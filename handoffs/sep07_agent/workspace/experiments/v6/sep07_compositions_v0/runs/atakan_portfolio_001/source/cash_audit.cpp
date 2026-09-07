#include "evaluation.hpp"
#include "registry.hpp"
#include <iomanip>

namespace audit {
#include "data.inc"
using Products=std::array<double,9>;
using Days=std::array<Products,30>;
struct Forecast {Products quantity{},revenue{};Days daily_quantity{},daily_revenue{};double own=0;};
double quote(int product,double inventory){
    const auto& p=kag::MARKET[product];const auto& a=kag::market_amps()[product];
    const double raw=inventory<p.I0?p.base+a.below*kag::shape(p.below_f,p.I0-inventory,p.T):p.base-a.above*kag::shape(p.above_f,inventory-p.I0,p.T);
    return std::max(1.,std::nearbyint(raw));
}
// Diagnostic copy of the frozen estimator. Equality with its public result is checked below.
Forecast forecast(const kag::agent::AgentObservation&o,int branch){
    Forecast out;auto& rival=out.daily_quantity;
    for(const auto&row:o.opponent().tiles)for(const auto&tile:row)if(tile.has_animal){
        const auto&d=kag::ANIMALS[tile.what-kag::GOOSE];int pending=tile.pending_care_bonus;
        rival[o.day][d.product]+=tile.yield_units;
        for(int day=o.day+1;day<30;++day){const int age=day-tile.planted_day;
            if(age>=d.first_yield_day&&(age-d.first_yield_day)%d.interval==0){rival[day][d.product]+=std::min(d.max_held,1+pending);pending=0;}++pending;
        }
    }
    Products stock{},known{},expected{};
    for(int p=0;p<9;++p){stock[p]=o.market.inventory[p];known[p]=1;
        for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<p))known[p]+=6*kag::SHOP_MULT[o.shops[i]];
        for(int shop=0;shop<8;++shop)if(kag::SHOP_MASK[shop]&(1u<<p))expected[p]+=6.*kag::SHOP_MULT[shop]/8.;
    }
    double total_rival=0;
    for(int day=o.day;day<30;++day){double profit=-planned_fixed[branch][day];
        const double fraction=day==o.day?(24.-o.hour)/24.:1.;const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
        for(int p=0;p<9;++p){const double demand=fraction*(known[p]+.35*unseen*expected[p]);
            double sales=planned_sales[branch][day][p],buys=planned_buys[branch][day][p];const double internal=std::min(sales,buys);sales-=internal;buys-=internal;
            const double mid=stock[p]+.5*(sales+rival[day][p]-buys-demand),sell_price=quote(p,mid),buy_price=quote(p,mid-1);
            profit+=sales*sell_price-buys*buy_price;out.daily_revenue[day][p]=rival[day][p]*sell_price;
            out.quantity[p]+=rival[day][p];out.revenue[p]+=out.daily_revenue[day][p];total_rival+=out.daily_revenue[day][p];
            stock[p]-=demand+buys;if(sell_price>1)stock[p]+=sales+rival[day][p];
        }out.own+=profit;
    }
    const auto check=compositions::atakan_portfolio::estimate(o,branch);
    if(check.own!=out.own||check.rival!=total_rival)std::abort();return out;
}
template<class T>void array(std::ostream&o,const T&v){o<<'[';bool comma=false;for(const auto&x:v){if(comma)o<<',';comma=true;o<<x;}o<<']';}
void days(std::ostream&o,const Days&v){o<<'[';for(int d=0;d<30;++d){if(d)o<<',';array(o,v[d]);}o<<']';}
struct Cash {Products revenue{},spend{},sold{},bought{},initial_shed{},initial_carried{},initial_public_held{},produced{};Days daily_revenue{},daily_spend{},daily_sold{};double fixed=0,cash_before=0;};
}
int main(int argc,char**argv){
    using namespace audit;const auto o=compositions::options(argc,argv);std::ofstream out(o.output);out<<std::setprecision(17)<<'[';bool comma=false;
    for(uint64_t seed:o.seeds)for(int seat=0;seat<2;++seat){
        if(o.seat_mode!=2&&seat!=o.seat_mode)continue;
        kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent(o.a),b=make_agent(o.b);
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        kag::agent::DecisionBudget budget;budget.max_expansions=o.expansions;std::array<uint8_t,8>shops{};uint64_t rng=seed^0xa37108e62d045fb9ULL;
        for(auto&shop:shops)shop=compositions::random_word(rng)%kag::N_SHOPS;
        Cash cash[2];std::array<Forecast,3>estimates{};uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
        std::vector<std::array<int,8>> visible_animals;std::vector<std::array<int,5>> visible_crops;
        while(!sim.st.done){
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);const auto oa=kag::agent::runtime::make_observation(sim,seat),ob=kag::agent::runtime::make_observation(sim,seat^1);
            if(sim.st.step==226){
                for(int branch=0;branch<3;++branch)estimates[branch]=forecast(oa,branch);
                for(int p=0;p<2;++p){const auto&f=sim.st.farms[p];cash[p].cash_before=f.money;
                    for(int item=0;item<9;++item){cash[p].initial_shed[item]=f.shed[item];cash[p].produced[item]=-f.produced[item];for(int u=0;u<f.n_units;++u)cash[p].initial_carried[item]+=f.inv[u][item];}
                    for(int y=0;y<10;++y)for(int x=0;x<10;++x){const auto&t=f.tiles[y][x];if(t.has_animal)cash[p].initial_public_held[kag::ANIMALS[t.what-kag::GOOSE].product]+=t.yield_units;else if(t.kind==kag::T_PLANT)cash[p].initial_public_held[t.what]+=t.yield_units;}
                }
                for(int y=0;y<10;++y)for(int x=0;x<10;++x){const auto&t=oa.opponent().tiles[y][x];if(t.has_animal)visible_animals.push_back({x,y,t.what,t.planted_day,t.yield_units,t.pending_care_bonus,t.fed_today,t.cared_today});else if(t.kind==kag::T_PLANT)visible_crops.push_back({x,y,t.what,t.planted_day,t.yield_units});}
            }
            kag::Action acts[2];a.act(oa,budget,acts[seat]);b.act(ob,budget,acts[seat^1]);compositions::validate_action(acts[seat],oa);compositions::validate_action(acts[seat^1],ob);
            for(int p=0;p<2;++p)compositions::hash_action(hashes[p],acts[p]);
            if(sim.st.step>=226){
                auto previous=sim;auto prefix0=acts[0],prefix1=acts[1];prefix0.n_orders=prefix1.n_orders=0;prefix0.finalize();prefix1.finalize();previous.step(prefix0,prefix1);
                const int slots=std::max(acts[0].n_orders,acts[1].n_orders);
                for(int i=0;i<slots;++i){auto next=sim;prefix0=acts[0];prefix1=acts[1];prefix0.n_orders=std::min(i+1,acts[0].n_orders);prefix1.n_orders=std::min(i+1,acts[1].n_orders);prefix0.finalize();prefix1.finalize();next.step(prefix0,prefix1);
                    int bought[2]{};bool has_buy=false;for(int p=0;p<2;++p)has_buy|=i<acts[p].n_orders&&acts[p].orders[i].op==kag::M_BUY_PRODUCT;
                    if(has_buy){auto prior0=prefix0,prior1=prefix1;prior0.n_orders=std::min(i,acts[0].n_orders);prior1.n_orders=std::min(i,acts[1].n_orders);prior0.finalize();prior1.finalize();
                        const auto before_orders=sim.diagnose_joint_actions(prior0,prior1),after_orders=sim.diagnose_joint_actions(prefix0,prefix1);
                        for(int p=0;p<2;++p)bought[p]=after_orders.players[p].successful_order_units-before_orders.players[p].successful_order_units;
                    }
                    for(int p=0;p<2;++p){const auto&f=next.st.farms[p];const auto&before=previous.st.farms[p];const double revenue=f.sell_revenue-before.sell_revenue,spend=f.total_spend-before.total_spend;
                        if(i>=acts[p].n_orders){if(revenue||spend)std::abort();continue;}
                        const auto&order=acts[p].orders[i];auto&c=cash[p];const int item=order.item,day=sim.st.day;
                        if(order.op==kag::M_SELL||order.op==kag::M_BUY_PRODUCT){if(item>=9)std::abort();c.revenue[item]+=revenue;c.spend[item]+=spend;c.daily_revenue[day][item]+=revenue;c.daily_spend[day][item]+=spend;
                            c.sold[item]+=f.sold_units[item]-before.sold_units[item];c.daily_sold[day][item]+=f.sold_units[item]-before.sold_units[item];if(order.op==kag::M_BUY_PRODUCT)c.bought[item]+=bought[p];
                        }else{if(revenue)std::abort();c.fixed+=spend;}
                    }previous=next;
                }
            }
            sim.step(acts[0],acts[1]);
        }
        if(comma)out<<',';comma=true;out<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"agent\":\""<<o.a<<"\",\"rival\":\""<<o.b<<"\",\"players\":[";
        for(int rel=0;rel<2;++rel){const int p=seat^rel;auto&c=cash[p];const auto&f=sim.st.farms[p];for(int item=0;item<9;++item)c.produced[item]+=f.produced[item];
            const double revenue=std::accumulate(c.revenue.begin(),c.revenue.end(),0.),spend=c.fixed+std::accumulate(c.spend.begin(),c.spend.end(),0.);if(f.money-c.cash_before!=revenue-spend)std::abort();
            if(rel)out<<',';out<<"{\"cash\":"<<f.money<<",\"cash_before226\":"<<c.cash_before<<",\"hash\":"<<hashes[p]<<",\"fixed_cost\":"<<c.fixed;
            const std::pair<const char*,Products*>arrays[]={{"revenue",&c.revenue},{"spend",&c.spend},{"sold",&c.sold},{"bought",&c.bought},{"produced_after226",&c.produced},{"private_shed_at226_OFFLINE_ONLY",&c.initial_shed},{"private_carried_at226_OFFLINE_ONLY",&c.initial_carried},{"public_held_at226",&c.initial_public_held}};
            for(const auto&[name,v]:arrays){out<<",\""<<name<<"\":";array(out,*v);}out<<",\"daily_revenue\":";days(out,c.daily_revenue);out<<",\"daily_sold\":";days(out,c.daily_sold);out<<'}';
        }out<<"],\"forecasts\":[";
        for(int branch=0;branch<3;++branch){if(branch)out<<',';const auto&e=estimates[branch];out<<"{\"own\":"<<e.own<<",\"rival_revenue\":";array(out,e.revenue);out<<",\"rival_quantity\":";array(out,e.quantity);out<<",\"daily_revenue\":";days(out,e.daily_revenue);out<<",\"daily_quantity\":";days(out,e.daily_quantity);out<<'}';}
        out<<"],\"rival_visible_animals_at226\":[";for(size_t i=0;i<visible_animals.size();++i){if(i)out<<',';array(out,visible_animals[i]);}out<<"],\"rival_visible_crops_at226\":[";for(size_t i=0;i<visible_crops.size();++i){if(i)out<<',';array(out,visible_crops[i]);}out<<"]}";
    }out<<"]\n";
}
