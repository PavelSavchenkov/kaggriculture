#include "source/policy.hpp"
#include "../../include/evaluation.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include "../../runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"
#include <chrono>
#include <fstream>
#include <iostream>

template<class Rival>
void probe(std::ostream& out,Rival& rival,const char* name,int mode,bool native,bool& first){
    compositions::observed_sale_lead_shared::Policy agent(mode);
    compositions::Options options;options.validate=true;options.native_shops=native;
    long long forecasts=0,matched=0,orders=0,units=0;double cash=0,margin=0;
    auto start=std::chrono::steady_clock::now();
    for(int s=0;s<32;++s)for(int seat=0;seat<2;++seat){
        auto result=compositions::run_game(agent,rival,(native?1954000:1000)+s,seat,options);
        const auto& d=agent.diagnostics();forecasts+=d.forecasts;matched+=d.matched;orders+=d.orders;units+=d.units;
        cash+=result.cash[seat];margin+=result.cash[seat]-result.cash[seat^1];
    }
    auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    if(!first)out<<",\n";first=false;
    out<<"{\"opponent\":\""<<name<<"\",\"mode\":"<<mode<<",\"native\":"<<native<<",\"games\":64,\"forecasts\":"<<forecasts
       <<",\"matched\":"<<matched<<",\"sale_orders\":"<<orders<<",\"sale_units\":"<<units<<",\"mean_cash\":"<<cash/64
       <<",\"mean_margin\":"<<margin/64<<",\"seconds\":"<<seconds<<'}';
}
int main(int argc,char** argv){
    if(argc!=2)return 2;std::ofstream out(argv[1]);bool first=true;out<<"[\n";
    kag::agents::rival_wool_context_v3::Agent current;kag::agents::public_router_v52::Agent v52;
    for(int mode=0;mode<3;++mode)for(bool native:{false,true}){
        probe(out,current,"rival_wool_context_v3",mode,native,first);
        probe(out,v52,"public_router_v52",mode,native,first);
    }
    out<<"\n]\n";
}
