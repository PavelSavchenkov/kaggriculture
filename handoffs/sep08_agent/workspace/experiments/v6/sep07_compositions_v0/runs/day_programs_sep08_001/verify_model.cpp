#include "../../include/evaluation.hpp"
#include "model.hpp"
#include "../joint_day_routes_sep08_001/proposals/joint_routes_p355_m0/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>

using namespace compositions;

bool same_own(const kag::agent::AgentObservation& a,const kag::agent::AgentObservation& b) {
    return a.player==b.player && a.step==b.step && a.day==b.day && a.hour==b.hour &&
        std::memcmp(&a.self(),&b.self(),sizeof(a.self()))==0 &&
        std::memcmp(&a.own,&b.own,sizeof(a.own))==0 &&
        std::memcmp(&a.market,&b.market,sizeof(a.market))==0;
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;std::filesystem::path dir=argv[1];
    if(std::filesystem::exists(dir))return 2;std::filesystem::create_directories(dir);
    int current_checks=0,step_checks=0,hidden_nonempty=0;
    for(int seed=1000;seed<1002;++seed)for(int seat=0;seat<2;++seat) {
        kag::Config config;config.seed=seed;kag::Sim sim(config);
        joint_routes_p355_m0::Agent own;public_router::Agent rival;day_programs::ObservationModel model;
        own.reset(kag::agent::runtime::make_agent_init(sim,seat));
        rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        model.reset(kag::agent::runtime::make_agent_config(sim.cfg));
        uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
        for(auto& shop:shops)shop=random_word(random)%kag::N_SHOPS;
        uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
        while(!sim.st.done) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            auto o=kag::agent::runtime::make_observation(sim,seat),r=kag::agent::runtime::make_observation(sim,seat^1);
            model.advance(o.step);auto prediction=model.make(o);
            if(!same_own(o,kag::agent::runtime::make_observation(prediction,seat)))std::abort();
            ++current_checks;
            for(int i=0;i<kag::N_ITEMS;++i)if(prediction.st.farms[seat^1].shed[i])std::abort();
            hidden_nonempty+=sim.st.farms[seat^1].shed_total>0;
            kag::Action actions[2],pass;
            own.act(o,{},actions[seat]);rival.act(r,{},actions[seat^1]);
            pass.clear();pass.n_units=r.self().n_units;pass.finalize();
            if(o.hour!=23) {
                auto expected=sim;
                if(seat==0){expected.step(actions[0],pass);prediction.step(actions[0],pass);}
                else{expected.step(pass,actions[1]);prediction.step(pass,actions[1]);}
                if(!same_own(kag::agent::runtime::make_observation(expected,seat),kag::agent::runtime::make_observation(prediction,seat))) {
                    std::cerr<<"projection mismatch seed="<<seed<<" seat="<<seat<<" step="<<o.step<<'\n';return 3;
                }
                ++step_checks;
            }
            for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
            sim.step(actions[0],actions[1]);
        }
        std::ofstream(dir/(std::to_string(seed)+"_s"+std::to_string(seat)+".json"))
            <<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash\":"<<sim.st.farms[seat].money
            <<",\"opponent_cash\":"<<sim.st.farms[seat^1].money<<",\"action_hash\":\""<<hashes[seat]
            <<"\",\"opponent_action_hash\":\""<<hashes[seat^1]<<"\"}\n";
    }
    std::ofstream(dir/"CHECKS.json")<<"{\"current_own_observations_exact\":"<<current_checks
        <<",\"daytime_solo_steps_exact\":"<<step_checks<<",\"nonempty_rival_stock_excluded_cases\":"<<hidden_nonempty<<"}\n";
    std::cout<<"current="<<current_checks<<" projected="<<step_checks<<" hidden excluded="<<hidden_nonempty<<'\n';
}
