#include "../include/evaluation.hpp"
#include "../include/deferred_animal.hpp"
#include "../runs/animal_entry_bank_001/bank.hpp"
#include "../candidates/shop_herd_guarded_001_best/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/teammate_shoprouter/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
using Source=shop_herd_guarded_001_best::Agent;
using Policy=DeferredAnimalAgent<Source>;

template<class Rival> void probe(const std::string& opponent,std::ofstream& out,double& elapsed,int& estimates) {
    const auto entries=animal_entry_bank_001::entries();const auto models=animal_entry_bank_001::models();
    for(int seed=1000;seed<1032;++seed)for(int seat=0;seat<2;++seat) {
        kag::Config config;config.seed=seed;kag::Sim sim(config);Policy own(265,7,32);Rival rival;
        own.reset(kag::agent::runtime::make_agent_init(sim,seat));rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        std::array<uint8_t,8> shops;uint64_t rng=seed^0xa37108e62d045fb9ULL;for(auto& shop:shops)shop=random_word(rng)%kag::N_SHOPS;
        while(sim.st.step<264) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);kag::Action pair[2];own.act(kag::agent::runtime::make_observation(sim,seat),{},pair[seat]);rival.act(kag::agent::runtime::make_observation(sim,seat^1),{},pair[seat^1]);sim.step(pair[0],pair[1]);
        }
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);const auto o=kag::agent::runtime::make_observation(sim,seat);
        if(o.day!=11 || o.hour!=0)std::abort();int context=-1;
        for(const auto& entry:entries)if(entry.day.plan.day==11 && entry.day.matches(o)){context=entry.model;break;}
        if(context<0)continue;
        const auto zero=value_animal_investment(o,models[context].flows,Biology{});
        for(const auto& entry:entries)if(entry.model==context) {
            const auto start=std::chrono::steady_clock::now();
            const auto biology=investment_biology(entry.item,entry.day.plan.day,models[context].service);
            const auto value=value_animal_investment(o,models[context].flows,biology);
            elapsed+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();++estimates;
            out<<"animal_d"<<entry.day.plan.day<<"_i"<<entry.item<<','<<opponent<<','<<seed<<','<<seat<<','<<context<<','<<value.own-zero.own<<','<<value.rival-zero.rival<<','<<value.margin()-zero.margin()<<','<<value.wheat<<','<<value.fertilizer<<','<<value.operations<<','<<value.first_output_day;
            for(int item=0;item<kag::N_PRODUCTS;++item){int produced=0;for(const auto& day:biology.days)produced+=day.output[item];out<<','<<produced;}out<<'\n';
        }
    }
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const std::filesystem::path folder=argv[1];if(std::filesystem::exists(folder))return 2;std::filesystem::create_directories(folder);
    std::ofstream out(folder/"estimates.csv");out<<"candidate,opponent,seed,seat,context,own_gain,rival_gain,margin_gain,wheat,fertilizer,operations,first_output_day";
    for(int i=0;i<kag::N_PRODUCTS;++i)out<<",output_"<<i;out<<'\n';double seconds=0;int count=0;
    probe<Source>("shop_herd_guarded_001_best",out,seconds,count);probe<public_router::Agent>("public_router",out,seconds,count);probe<king_rc4::Agent>("king_rc4",out,seconds,count);probe<opening_router_v4::Agent>("opening_router_v4",out,seconds,count);probe<teammate_shoprouter::Agent>("teammate_shoprouter",out,seconds,count);
    std::ofstream(folder/"timing.json")<<"{\"estimates\":"<<count<<",\"seconds\":"<<seconds<<",\"microseconds_per_estimate\":"<<1e6*seconds/count<<",\"scope\":\"Candidate biology plus complete daily whole-farm market forecast, one CPU thread; excludes prefix generation\"}\n";
    std::cout<<count<<" estimates "<<1e6*seconds/count<<"us/estimate\n";
}
