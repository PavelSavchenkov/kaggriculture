#include "evaluation.hpp"
#include "registry.hpp"
#include <iomanip>

namespace oracle {
using Products=std::array<double,9>;
using Days=std::array<Products,30>;
using Costs=std::array<double,30>;
struct Plan {Days sales{},buys{};Costs fixed{};};
struct Truth {Plan own,rival;};
struct Value {double own=0,rival=0;Products own_revenue{},own_spend{},rival_revenue{},rival_spend{};Days quotes{};};
double quote(int product,double inventory){
    const auto&p=kag::MARKET[product];const auto&a=kag::market_amps()[product];
    const double raw=inventory<p.I0?p.base+a.below*kag::shape(p.below_f,p.I0-inventory,p.T):p.base-a.above*kag::shape(p.above_f,inventory-p.I0,p.T);
    return std::max(1.,std::nearbyint(raw));
}
Days herd(const kag::agent::AgentObservation&o){
    Days result{};
    for(const auto&row:o.opponent().tiles)for(const auto&tile:row)if(tile.has_animal){
        const auto&d=kag::ANIMALS[tile.what-kag::GOOSE];int pending=tile.pending_care_bonus;result[o.day][d.product]+=tile.yield_units;
        for(int day=o.day+1;day<30;++day){const int age=day-tile.planted_day;
            if(age>=d.first_yield_day&&(age-d.first_yield_day)%d.interval==0){result[day][d.product]+=std::min(d.max_held,1+pending);pending=0;}++pending;
        }
    }return result;
}
// Bits 1/2/4 replace rival flows / own flows / unknown shops. Bit8 supplies
// rival fixed cost. Bit16 supplies the exact public consumption calendar.
// All future quantities and shops are OFFLINE ORACLES, never policy inputs.
Value value(const kag::agent::AgentObservation&o,const Plan&donor,const Truth&truth,const std::array<int,8>&shops,int mask){
    Value out;Plan other;other.sales=herd(o);const Plan&own=(mask&2)?truth.own:donor;
    if(mask&1){other.sales=truth.rival.sales;other.buys=truth.rival.buys;}
    if(mask&8)other.fixed=truth.rival.fixed;
    Products stock{},known{},expected{};
    for(int p=0;p<9;++p){stock[p]=o.market.inventory[p];known[p]=1;
        for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<p))known[p]+=6*kag::SHOP_MULT[o.shops[i]];
        for(int shop=0;shop<8;++shop)if(kag::SHOP_MASK[shop]&(1u<<p))expected[p]+=6.*kag::SHOP_MULT[shop]/8.;
    }
    for(int day=o.day;day<30;++day){double own_profit=-own.fixed[day],rival_profit=-other.fixed[day];
        const double fraction=day==o.day?(24.-o.hour)/24.:1.;const int unseen=std::max(0,std::min(8,day/3)-o.n_shops);
        for(int p=0;p<9;++p){double demand=fraction*(known[p]+.35*unseen*expected[p]);
            if(mask&4){double full_day=known[p];for(int i=o.n_shops;i<o.n_shops+unseen;++i)if(kag::SHOP_MASK[shops[i]]&(1u<<p))full_day+=6*kag::SHOP_MULT[shops[i]];demand=fraction*full_day;}
            if(mask&16){demand=0;for(int step=std::max(o.step,day*24);step<std::min(719,(day+1)*24);++step){
                    if(step%24==0&&p<kag::FERTILIZER)demand+=1;
                    if(step%4==0)for(int i=0;i<std::min(8,day/3);++i)if(kag::SHOP_MASK[shops[i]]&(1u<<p))demand+=kag::SHOP_MULT[shops[i]];
                }}
            double sales=own.sales[day][p],buys=own.buys[day][p],rival_sales=other.sales[day][p],rival_buys=other.buys[day][p];
            const double internal=std::min(sales,buys),rival_internal=std::min(rival_sales,rival_buys);sales-=internal;buys-=internal;rival_sales-=rival_internal;rival_buys-=rival_internal;
            const double mid=stock[p]+.5*(sales+rival_sales-buys-rival_buys-demand),sell_price=quote(p,mid),buy_price=quote(p,mid-1);
            out.quotes[day][p]=sell_price;const double revenue=sales*sell_price,spend=buys*buy_price,rival_revenue=rival_sales*sell_price,rival_spend=rival_buys*buy_price;
            own_profit+=revenue-spend;rival_profit+=rival_revenue-rival_spend;
            out.own_revenue[p]+=revenue;out.own_spend[p]+=spend;out.rival_revenue[p]+=rival_revenue;out.rival_spend[p]+=rival_spend;
            stock[p]-=demand+buys+rival_buys;if(sell_price>1)stock[p]+=sales+rival_sales;
        }out.own+=own_profit;out.rival+=rival_profit;
    }return out;
}
template<class T>void array(std::ostream&o,const T&v){o<<'[';bool comma=false;for(const auto&x:v){if(comma)o<<',';comma=true;o<<x;}o<<']';}
template<class T>void read(std::istream&i,T&v){for(auto&x:v)i>>x;}
void read(std::istream&i,Days&v){for(auto&row:v)read(i,row);}
void days(std::ostream&o,const Days&v){o<<'[';for(int d=0;d<30;++d){if(d)o<<',';array(o,v[d]);}o<<']';}
}
int main(int argc,char**argv){
    if(argc!=3)std::abort();using namespace oracle;std::ifstream input(argv[1]);std::ofstream output(argv[2]);if(!input||!output)std::abort();
    Plan donors[3];for(auto&d:donors){read(input,d.sales);read(input,d.buys);read(input,d.fixed);}int count;input>>count;output<<std::setprecision(17)<<'[';
    for(int c=0;c<count;++c){std::string rival;uint64_t seed;int seat;std::array<int,8>shops;Truth truth[3];input>>rival>>seed>>seat;read(input,shops);
        for(auto&t:truth){read(input,t.own.sales);read(input,t.own.buys);read(input,t.rival.sales);read(input,t.rival.buys);read(input,t.own.fixed);read(input,t.rival.fixed);}if(!input)std::abort();
        kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent("atakan_cow"),b=make_agent(rival);
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));kag::agent::DecisionBudget budget;budget.max_expansions=100000;
        for(int step=0;step<226;++step){if(sim.st.n_shops<0||sim.st.n_shops>8)std::abort();for(int i=0;i<8;++i)if(i<sim.st.n_shops)sim.st.shops[i]=shops[i];
            const auto oa=kag::agent::runtime::make_observation(sim,seat),ob=kag::agent::runtime::make_observation(sim,seat^1);
            kag::Action acts[2];a.act(oa,budget,acts[seat]);b.act(ob,budget,acts[seat^1]);compositions::validate_action(acts[seat],oa);compositions::validate_action(acts[seat^1],ob);sim.step(acts[0],acts[1]);
        }
        if(sim.st.n_shops<0||sim.st.n_shops>8)std::abort();for(int i=0;i<8;++i)if(i<sim.st.n_shops)sim.st.shops[i]=shops[i];const auto observation=kag::agent::runtime::make_observation(sim,seat);
        if(c)output<<',';output<<"{\"rival\":\""<<rival<<"\",\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash_before226\":"<<observation.self().money<<",\"rival_cash_before226\":"<<observation.opponent().money;
        output<<",\"physical_before226\":";array(output,compositions::atakan_portfolio::physical(observation));output<<",\"market_inventory\":";array(output,observation.market.inventory);
        output<<",\"variants\":[";const int masks[]={0,1,2,3,4,5,6,7,15,31};
        for(size_t v=0;v<std::size(masks);++v){if(v)output<<',';output<<"{\"mask\":"<<masks[v]<<",\"branches\":[";
            for(int branch=0;branch<3;++branch){const auto result=value(observation,donors[branch],truth[branch],shops,masks[v]);
                if(masks[v]==0){const auto baseline=compositions::atakan_portfolio::estimate(observation,branch);if(result.own!=baseline.own||result.rival!=baseline.rival)std::abort();}
                if(branch)output<<',';output<<"{\"own\":"<<result.own<<",\"rival\":"<<result.rival;
                const std::pair<const char*,const Products*>arrays[]={{"own_revenue",&result.own_revenue},{"own_spend",&result.own_spend},{"rival_revenue",&result.rival_revenue},{"rival_spend",&result.rival_spend}};
                for(const auto&[name,data]:arrays){output<<",\""<<name<<"\":";array(output,*data);}
                if(seed==1028&&seat==0&&rival=="public_router_v5"){output<<",\"daily_quotes\":";days(output,result.quotes);}output<<'}';
            }output<<"]}";
        }output<<"]}";
    }output<<"]\n";std::string excess;if(input>>excess)std::abort();
}
