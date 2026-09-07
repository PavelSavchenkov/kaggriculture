#include "evaluation.hpp"
#include "registry.hpp"
#include <iomanip>

int main(int argc,char**argv){
    const auto o=compositions::options(argc,argv);std::ofstream out(o.output);out<<std::setprecision(17)<<'[';bool comma=false;
    for(uint64_t seed:o.seeds)for(int seat=0;seat<2;++seat){
        kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent("atakan_cow"),b=make_agent(o.b);
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));kag::agent::DecisionBudget budget;budget.max_expansions=o.expansions;
        std::array<uint8_t,8>shops{};uint64_t rng=seed^0xa37108e62d045fb9ULL;for(auto&shop:shops)shop=compositions::random_word(rng)%kag::N_SHOPS;
        for(int step=0;step<226;++step){if(sim.st.n_shops<0||sim.st.n_shops>8)std::abort();for(int i=0;i<8;++i)if(i<sim.st.n_shops)sim.st.shops[i]=shops[i];
            const auto oa=kag::agent::runtime::make_observation(sim,seat),ob=kag::agent::runtime::make_observation(sim,seat^1);kag::Action acts[2];
            a.act(oa,budget,acts[seat]);b.act(ob,budget,acts[seat^1]);compositions::validate_action(acts[seat],oa);compositions::validate_action(acts[seat^1],ob);sim.step(acts[0],acts[1]);}
        const auto observation=kag::agent::runtime::make_observation(sim,seat);
        if(comma)out<<',';comma=true;out<<"{\"rival\":\""<<o.b<<"\",\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash_before226\":"<<observation.self().money<<",\"rival_cash_before226\":"<<observation.opponent().money<<",\"variants\":[";
        const int counts[]={0,1,8,32,64};
        for(int i=0;i<5;++i){const int count=counts[i];const auto estimates=compositions::atakan_integrated::estimate_all(observation,count);
            if(count==0)for(int branch=0;branch<3;++branch){const auto old=compositions::atakan_portfolio::estimate(observation,branch);if(estimates[branch].own!=old.own||estimates[branch].rival!=old.rival||estimates[branch].min_cash!=old.min_cash)std::abort();}
            // Actual three-branch forecast, including sample generation. A compiler
            // memory barrier prevents reuse across the eight timing repetitions.
            volatile double sink=0;const auto started=std::chrono::steady_clock::now();
            for(int repeat=0;repeat<8;++repeat){asm volatile("":::"memory");const auto v=compositions::atakan_integrated::estimate_all(observation,count);sink=sink+v[repeat%3].own;}
            const double micros=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-started).count()/8;
            if(i)out<<',';out<<"{\"count\":"<<count<<",\"microseconds_per_decision\":"<<micros<<",\"branches\":[";
            for(int branch=0;branch<3;++branch){if(branch)out<<',';const auto&v=estimates[branch];out<<"{\"own\":"<<v.own<<",\"rival\":"<<v.rival<<",\"min_cash\":"<<v.min_cash<<'}';}out<<"]}";
        }out<<"]}";
    }out<<"]\n";
}
