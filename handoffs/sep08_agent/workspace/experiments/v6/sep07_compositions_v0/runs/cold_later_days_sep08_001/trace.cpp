#include "../../include/evaluation.hpp"
#include "../../include/biology.hpp"
#include "../cold_day_tasks_sep08_001/policy.hpp"
#include "../../league/public_router/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

void snapshot(std::ostream& file,const kag::Sim& sim,int seat,const char* phase){
    const auto& f=sim.st.farms[seat];
    file<<"{\"phase\":\""<<phase<<"\",\"step\":"<<sim.st.step<<",\"day\":"<<sim.st.day
        <<",\"cash\":"<<f.money<<",\"hires\":"<<f.hires_today<<",\"quadrants\":"<<f.n_quadrants<<",\"produced\":[";
    for(int i=0;i<kag::N_PRODUCTS;++i){if(i)file<<',';file<<f.produced[i];}
    file<<"],\"shed\":[";for(int i=0;i<kag::N_ITEMS;++i){if(i)file<<',';file<<f.shed[i];}
    file<<"],\"seeds\":[";for(int i=0;i<kag::N_CROPS;++i){if(i)file<<',';file<<f.seeds[i];}
    file<<"],\"tiles\":[";
    for(int c=0;c<100;++c){const auto& t=f.tiles[c/10][c%10];if(c)file<<',';
        file<<'['<<+t.kind<<','<<+t.what<<','<<t.has_animal<<','<<t.planted_day<<','<<+t.yield_units<<','<<+t.consecutive_dry
            <<','<<t.watered_today<<','<<t.fed_today<<','<<t.cared_today<<','<<t.fertilizer_available<<','<<+t.pending_care_bonus<<','<<t.fertilized_until_day<<']';}
    file<<"]}\n";
}

template<int Mode> void trace(const fs::path& directory,uint64_t seed,int seat){
    const std::string name="mode"+std::to_string(Mode)+"_"+std::to_string(seed)+"_s"+std::to_string(seat);
    std::ofstream snapshots(directory/(name+".jsonl"));
    kag::Config config;config.seed=seed;kag::Sim sim(config);
    cold_day_tasks::Agent<Mode> own;public_router::Agent rival;
    own.reset(kag::agent::runtime::make_agent_init(sim,seat));rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
    uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
    for(auto& s:shops)s=random_word(random)%kag::N_SHOPS;
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done){
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        if(sim.st.hour==0)snapshot(snapshots,sim,seat,"start");
        if(sim.st.hour==23 || sim.st.step==718)snapshot(snapshots,sim,seat,"before_last_hour");
        kag::Action actions[2];const auto a=kag::agent::runtime::make_observation(sim,seat),b=kag::agent::runtime::make_observation(sim,seat^1);
        own.act(a,{},actions[seat]);rival.act(b,{},actions[seat^1]);
        validate_action(actions[seat],a);validate_action(actions[seat^1],b);
        for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
        sim.step(actions[0],actions[1]);
    }
    snapshot(snapshots,sim,seat,"terminal");
    std::ofstream end(directory/(name+".json"));
    end<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"mode\":"<<Mode<<",\"cash\":"<<sim.st.farms[seat].money
        <<",\"opponent_cash\":"<<sim.st.farms[seat^1].money<<",\"action_hash\":\""<<hashes[seat]<<"\",\"opponent_action_hash\":\""<<hashes[seat^1]<<"\"}\n";
}

int main(int argc,char** argv){
    if(argc!=2)return 2;const fs::path dir=argv[1];if(fs::exists(dir))return 2;fs::create_directories(dir);
    for(int seed=1000;seed<1004;++seed)for(int seat=0;seat<2;++seat){trace<-1>(dir,seed,seat);trace<0>(dir,seed,seat);trace<2>(dir,seed,seat);}
    std::ofstream expected(dir/"expected.json");expected<<"{\"daily_output\":[";
    int output[30][kag::N_PRODUCTS]{};
    for(const auto& life:cold_day_tasks::lives){
        Cohort c{uint8_t(life.item),1,life.start/24,std::min(30,(life.end+23)/24)};
        auto s=productive_service(c,life.item==kag::STRAWBERRY || life.item==kag::TOMATO);
        const auto b=biology(c,s);
        for(int d=0;d<30;++d)for(int i=0;i<kag::N_PRODUCTS;++i)output[d][i]+=b.days[d].output[i];
    }
    for(int d=0;d<30;++d){if(d)expected<<',';expected<<'[';for(int i=0;i<kag::N_PRODUCTS;++i){if(i)expected<<',';expected<<output[d][i];}expected<<']';}
    expected<<"],\"scope\":\"Existing productive biological estimator on requested dates; service differs from compiler and may not be funded. Not an exact attainable output promise.\"}\n";
}
