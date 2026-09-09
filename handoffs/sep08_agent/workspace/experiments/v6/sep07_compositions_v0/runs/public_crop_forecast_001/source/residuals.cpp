#define main branch_forecast_main
#include "branch_forecast.cpp"
#undef main
#include <chrono>

int main(int argc,char**argv){
    if(argc!=3)return 2;using namespace diagnostic;
    std::ifstream input(argv[1]);std::ofstream out(argv[2]);out<<std::setprecision(17);
    int branches;input>>branches;std::vector<Plan>plans(branches);
    for(auto&p:plans){read(input,p.sales);read(input,p.buys);read(input,p.fixed);}
    int count;input>>count;out<<'[';
    for(int c=0;c<count;++c){
        std::string rival;uint64_t seed;int seat;std::array<int,8>shops;input>>rival>>seed>>seat;read(input,shops);if(!input)return 3;
        kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent(FIXED_POLICY),b=make_agent(rival);
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));kag::agent::DecisionBudget budget;
        for(int step=0;step<PREFIX_STEP;++step){std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);kag::Action actions[2];
            a.act(kag::agent::runtime::make_observation(sim,seat),budget,actions[seat]);b.act(kag::agent::runtime::make_observation(sim,seat^1),budget,actions[seat^1]);sim.step(actions[0],actions[1]);}
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);const auto o=kag::agent::runtime::make_observation(sim,seat);const auto animals=herd(o);
        if(c)out<<',';out<<"{\"rival\":\""<<rival<<"\",\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"modes\":[";
        bool comma=false;
        for(int mode:{0,2,4}){
            const auto start=std::chrono::steady_clock::now();const auto crops=crop_output(o,Mode(mode)).output;
            auto flow=animals;for(int day=0;day<30;++day)for(int p=0;p<9;++p)flow[day][p]+=crops[day][p];
            std::vector<Value>values(branches);for(int sample=0;sample<64;++sample){const auto d=demand(o,64,sample);for(int branch=0;branch<branches;++branch){const auto v=value(o,plans[branch],flow,d);values[branch].own+=v.own/64;values[branch].rival+=v.rival/64;}}
            const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
            if(comma)out<<',';comma=true;out<<"{\"mode\":"<<mode<<",\"nanoseconds\":"<<ns<<",\"rival_flow\":[";
            for(int day=0;day<30;++day){if(day)out<<',';array(out,flow[day]);}out<<"],\"values\":[";
            for(int branch=0;branch<branches;++branch){if(branch)out<<',';out<<"{\"own\":"<<values[branch].own<<",\"rival\":"<<values[branch].rival<<'}';}out<<']';
            if(seed==1028 && rival=="public_router_v5"){
                out<<",\"product_values\":[";
                for(int branch=0;branch<branches;++branch){if(branch)out<<',';out<<'[';
                    for(int product=0;product<9;++product){Plan own;Days other{};for(int day=0;day<30;++day){own.sales[day][product]=plans[branch].sales[day][product];own.buys[day][product]=plans[branch].buys[day][product];other[day][product]=flow[day][product];}
                        Value total;for(int sample=0;sample<64;++sample){const auto v=value(o,own,other,demand(o,64,sample));total.own+=v.own/64;total.rival+=v.rival/64;}
                        if(product)out<<',';out<<"{\"own\":"<<total.own<<",\"rival\":"<<total.rival<<'}';}out<<']';}out<<']';}
            out<<'}';
        }out<<"]}";
    }out<<"]\n";
}
