#include "../../include/evaluation.hpp"
#include "source/agent.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

void orders(std::ostream& out,const kag::Action& action) {
    out<<'[';
    for(int i=0;i<action.n_orders;++i) {
        if(i)out<<',';const auto& o=action.orders[i];out<<'['<<+o.op<<','<<+o.item<<','<<o.n<<']';
    }
    out<<']';
}

void trace(const fs::path& dir,int seed,int seat,int mode) {
    std::string name="m"+std::to_string(mode)+"_"+std::to_string(seed)+"_s"+std::to_string(seat);
    std::ofstream out(dir/(name+".jsonl"));
    kag::Config config;config.seed=seed;kag::Sim sim(config);
    arlene_v4_sep08::AgentCore own(mode);kag::agents::public_router_v52::Agent rival;
    own.reset(kag::agent::runtime::make_agent_init(sim,seat));rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
    uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
    for(auto& shop:shops)shop=random_word(random)%kag::N_SHOPS;
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done) {
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        kag::Action actions[2];auto a=kag::agent::runtime::make_observation(sim,seat),b=kag::agent::runtime::make_observation(sim,seat^1);
        own.act(a,{},actions[seat]);rival.act(b,{},actions[seat^1]);
        validate_action(actions[seat],a);validate_action(actions[seat^1],b);
        out<<"{\"step\":"<<sim.st.step<<",\"own\":["<<a.self().money<<','<<a.self().n_units<<','<<a.self().hires_today
            <<"],\"rival\":["<<b.self().money<<','<<b.self().n_units<<','<<b.self().hires_today<<"],\"inventory\":[";
        for(int i=0;i<kag::N_PRODUCTS;++i){if(i)out<<',';out<<a.market.inventory[i];}
        out<<"],\"shed\":[";for(int i=0;i<kag::N_ITEMS;++i){if(i)out<<',';out<<+a.own.shed[i];}
        out<<"],\"own_orders\":";orders(out,actions[seat]);out<<",\"rival_orders\":";orders(out,actions[seat^1]);
        out<<",\"route\":"<<rival.selected_route()<<"}\n";
        for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
        sim.step(actions[0],actions[1]);
    }
    std::ofstream end(dir/(name+".json"));
    end<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"mode\":"<<mode<<",\"cash\":"<<sim.st.farms[seat].money
        <<",\"opponent_cash\":"<<sim.st.farms[seat^1].money<<",\"action_hash\":\""<<hashes[seat]
        <<"\",\"opponent_action_hash\":\""<<hashes[seat^1]<<"\"}\n";
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;fs::path dir=argv[1];if(fs::exists(dir))return 2;fs::create_directories(dir);
    for(int seed=1000;seed<1002;++seed)for(int seat=0;seat<2;++seat)for(int mode:{0,2})trace(dir,seed,seat,mode);
}
