#include "forecast.hpp"
#include "registry.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>

int main(int argc,char** argv) {
    if(argc!=2||std::filesystem::exists(argv[1]))return 2;
    using namespace compositions;
    std::ofstream output(argv[1]);output<<std::setprecision(17);
    output<<"opponent,seed,seat,mode,product,predicted,actual,absolute_quantity_error,dated_absolute_error,forecast_microseconds\n";
    for(const std::string opponent:{"crop_mix_t2_wheat","wheat_one_fert","teammate_shoprouter","public_router","king_rc4","public_router_v5"}) {
        auto own=make_agent("crop_mix_t2_wheat"),rival=make_agent(opponent);
        for(uint64_t seed=1000;seed<1032;++seed)for(int seat=0;seat<2;++seat) {
            kag::Config config;config.seed=seed;kag::Sim sim(config);
            own.reset(kag::agent::runtime::make_agent_init(sim,seat));
            rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
            uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops{};
            for(auto& shop:shops)shop=random_word(random)%kag::N_SHOPS;
            std::array<crop_choice::Days,4> forecasts{};crop_choice::Days actual{};
            double microseconds=0;bool captured=false;
            while(!sim.st.done) {
                std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
                const auto observation=kag::agent::runtime::make_observation(sim,seat);
                if(observation.day==12&&observation.hour==0) {
                    const auto begin=std::chrono::steady_clock::now();
                    for(int mode=0;mode<4;++mode)forecasts[mode]=crop_choice::rival_flows(observation,mode,false,false);
                    microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
                    captured=true;
                }
                kag::Action actions[2];own.act(observation,{},actions[seat]);
                rival.act(kag::agent::runtime::make_observation(sim,seat^1),{},actions[seat^1]);
                validate_action(actions[seat],observation);
                const int day=sim.st.day;std::array<int,5> before{};
                for(int product=0;product<5;++product)before[product]=sim.st.farms[seat^1].produced[product];
                sim.step(actions[0],actions[1]);
                if(captured)for(int product=0;product<5;++product)
                    actual[day][product]+=sim.st.farms[seat^1].produced[product]-before[product];
            }
            if(!captured)std::abort();
            for(int mode=0;mode<4;++mode)for(int product=0;product<5;++product) {
                double predicted=0,real=0,dated_error=0;
                for(int day=12;day<30;++day) {
                    predicted+=forecasts[mode][day][product];real+=actual[day][product];
                    dated_error+=std::abs(forecasts[mode][day][product]-actual[day][product]);
                }
                output<<opponent<<','<<seed<<','<<seat<<','<<mode<<','<<product<<','<<predicted<<','<<real<<','
                      <<std::abs(predicted-real)<<','<<dated_error<<','<<microseconds<<'\n';
            }
        }
        output.flush();std::cout<<opponent<<" 64 full-game forecast audits\n";
    }
}
