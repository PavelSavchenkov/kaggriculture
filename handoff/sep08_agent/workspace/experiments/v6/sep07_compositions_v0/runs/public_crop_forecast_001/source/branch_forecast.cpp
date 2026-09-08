#include "crop_forecast.hpp"
#include "evaluation.hpp"
#include "registry.hpp"
#include <iomanip>
#include <iostream>

namespace diagnostic {
using namespace compositions::public_crop_forecast;
struct Plan {Days sales{},buys{};std::array<double,30>fixed{};};
struct Value {double own=0,rival=0;};
uint64_t random(uint64_t&s){s+=0x9e3779b97f4a7c15ULL;uint64_t x=s;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
double quote(int product,double inventory){const auto&p=kag::MARKET[product];const auto&a=kag::market_amps()[product];const double raw=inventory<p.I0?p.base+a.below*kag::shape(p.below_f,p.I0-inventory,p.T):p.base-a.above*kag::shape(p.above_f,inventory-p.I0,p.T);return std::max(1.,std::nearbyint(raw));}
Days herd(const kag::agent::AgentObservation&o){Days result{};for(const auto&row:o.opponent().tiles)for(const auto&t:row)if(t.has_animal){const auto&a=kag::ANIMALS[t.what-kag::GOOSE];int pending=t.pending_care_bonus;result[o.day][a.product]+=t.yield_units;for(int day=o.day+1;day<30;++day){const int age=day-t.planted_day;if(age>=a.first_yield_day&&(age-a.first_yield_day)%a.interval==0){result[day][a.product]+=std::min(a.max_held,1+pending);pending=0;}++pending;}}return result;}
Days demand(const kag::agent::AgentObservation&o,int integration,int sample){
    Days result{};std::array<int,8>shops{};
    if(integration>1)for(int reveal=0;reveal<8;++reveal){if(reveal<o.n_shops){shops[reveal]=o.shops[reveal];continue;}std::array<int,8>p{0,1,2,3,4,5,6,7};uint64_t state=uint64_t(sample/8+1)*0xd1b54a32d192ed03ULL^uint64_t(reveal+1)*0x94d049bb133111ebULL;for(int i=7;i>0;--i)std::swap(p[i],p[random(state)%(i+1)]);shops[reveal]=p[sample%8];}
    for(int day=o.day;day<30;++day)for(int product=0;product<9;++product){
        const double fraction=day==o.day?(24.-o.hour)/24.:1.;const int reveal_count=std::max(o.n_shops,std::min(8,day/3));double quantity=FERT_CENTER || product!=kag::FERTILIZER;
        for(int reveal=0;reveal<(integration>1?reveal_count:o.n_shops);++reveal){const int shop=integration>1?shops[reveal]:o.shops[reveal];if(kag::SHOP_MASK[shop]&(1u<<product))quantity+=6*kag::SHOP_MULT[shop];}
        if(integration<2){double expected=0;for(int shop=0;shop<8;++shop)if(kag::SHOP_MASK[shop]&(1u<<product))expected+=6.*kag::SHOP_MULT[shop]/8.;quantity+=(integration?1.:.35)*(reveal_count-o.n_shops)*expected;}
        result[day][product]=fraction*quantity;
    }return result;
}
Value value(const kag::agent::AgentObservation&o,const Plan&own,const Days&rival,const Days&consume){
    Value result;Products stock{};for(int p=0;p<9;++p)stock[p]=o.market.inventory[p];
    for(int day=o.day;day<30;++day){double profit=-own.fixed[day];for(int p=0;p<9;++p){
        double sell=own.sales[day][p],buy=own.buys[day][p];const double internal=std::min(sell,buy);sell-=internal;buy-=internal;
        const double other_sell=std::max(0.,rival[day][p]),other_buy=std::max(0.,-rival[day][p]),mid=stock[p]+.5*(sell+other_sell-buy-other_buy-consume[day][p]),price=quote(p,mid),buy_price=quote(p,mid-1);
        profit+=sell*price-buy*buy_price;result.rival+=other_sell*price-other_buy*buy_price;stock[p]-=consume[day][p]+buy+other_buy;if(price>1)stock[p]+=sell+other_sell;
    }result.own+=profit;}return result;
}
template<class T>void read(std::istream&i,T&v){for(auto&x:v)i>>x;}
void read(std::istream&i,Days&v){for(auto&row:v)read(i,row);}
template<class T>void array(std::ostream&o,const T&v){o<<'[';bool first=true;for(const auto&x:v){if(!first)o<<',';first=false;o<<x;}o<<']';}
}

int main(int argc,char**argv){
    if(argc!=3)return 2;using namespace diagnostic;std::ifstream input(argv[1]);std::ofstream out(argv[2]);out<<std::setprecision(17);
    int branches;input>>branches;std::vector<Plan>plans(branches);for(auto&p:plans){read(input,p.sales);read(input,p.buys);read(input,p.fixed);}int count;input>>count;out<<'[';
    for(int c=0;c<count;++c){std::string rival;uint64_t seed;int seat;std::array<int,8>shops;input>>rival>>seed>>seat;read(input,shops);if(!input)return 3;
        kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent(FIXED_POLICY),b=make_agent(rival);a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        kag::agent::DecisionBudget budget;budget.max_expansions=100000;
        for(int step=0;step<PREFIX_STEP;++step){std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);kag::Action actions[2];a.act(kag::agent::runtime::make_observation(sim,seat),budget,actions[seat]);b.act(kag::agent::runtime::make_observation(sim,seat^1),budget,actions[seat^1]);sim.step(actions[0],actions[1]);}
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);const auto o=kag::agent::runtime::make_observation(sim,seat);const auto animals=herd(o);
        if(c)out<<',';out<<"{\"case\":"<<c<<",\"rival\":\""<<rival<<"\",\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"own_cash\":"<<o.self().money<<",\"rival_cash\":"<<o.opponent().money<<",\"physical\":";
#ifdef ATAKAN
        array(out,compositions::atakan_portfolio::physical(o));
#else
        array(out,compositions::sheep_portfolio::physical(o));
#endif
        out<<",\"variants\":[";bool comma=false;
        for(int integration:{0,1,64}){std::vector<Days>demands;for(int sample=0;sample<std::max(1,integration);++sample)demands.push_back(demand(o,integration,sample));
            for(int mode=0;mode<=5;++mode)for(int net=0;net<=(mode>0);++net){
                auto crops=crop_output(o,Mode(mode)).output;if(net)net_visible_herd_feed(o,crops);auto flow=animals;for(int day=0;day<30;++day)for(int p=0;p<9;++p)flow[day][p]+=crops[day][p];
                std::vector<Value>values(branches);for(const auto&d:demands)for(int branch=0;branch<branches;++branch){const auto v=value(o,plans[branch],flow,d);values[branch].own+=v.own;values[branch].rival+=v.rival;}
                for(auto&v:values){v.own/=demands.size();v.rival/=demands.size();}
                if(mode==0 && integration==0){
#ifdef ATAKAN
                    for(int branch=0;branch<branches;++branch){const auto old=compositions::atakan_portfolio::estimate(o,branch);if(values[branch].own!=old.own || values[branch].rival!=old.rival)std::abort();}
#endif
                }
#ifndef ATAKAN
                if(mode==0){const auto old=compositions::sheep_portfolio::estimate_all(o,integration);for(int branch=0;branch<branches;++branch)if(values[branch].own!=old[branch].own || values[branch].rival!=old[branch].rival)std::abort();}
#endif
                if(comma)out<<',';comma=true;out<<"{\"integration\":"<<integration<<",\"crop_mode\":"<<mode<<",\"feed_net\":"<<net<<",\"branches\":[";for(int branch=0;branch<branches;++branch){if(branch)out<<',';out<<"{\"own\":"<<values[branch].own<<",\"rival\":"<<values[branch].rival<<'}';}out<<"]}";
            }
        }out<<"]}";
    }out<<"]\n";std::cout<<"branch_contexts="<<count<<"\n";
}
