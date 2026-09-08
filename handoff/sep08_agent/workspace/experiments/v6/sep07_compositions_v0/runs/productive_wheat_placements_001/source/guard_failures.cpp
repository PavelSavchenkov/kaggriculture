#include "evaluation.hpp"
#include "registry.hpp"
int main(int argc,char**argv){
    if(argc!=3)return 2;std::ifstream input(argv[1]);std::ofstream out(argv[2]);int count;input>>count;out<<'[';
    for(int c=0;c<count;++c){std::string rival;uint64_t seed;int seat;input>>rival>>seed>>seat;
        auto a=make_agent("wheat_one_fert"),b=make_agent(rival);kag::Config cfg;cfg.seed=seed;kag::Sim sim(cfg);
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8>shops;
        for(auto&shop:shops)shop=compositions::random_word(random)%kag::N_SHOPS;
        while(sim.st.day<28){std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);kag::Action pair[2];
            a.act(kag::agent::runtime::make_observation(sim,seat),{},pair[seat]);b.act(kag::agent::runtime::make_observation(sim,seat^1),{},pair[seat^1]);sim.step(pair[0],pair[1]);}
        const auto o=kag::agent::runtime::make_observation(sim,seat);const auto g=compositions::wheat_one_fert::off_days().back();
        if(c)out<<',';out<<"{\"rival\":\""<<rival<<"\",\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash\":"<<o.self().money<<",\"differences\":[";bool comma=false;
        auto difference=[&](const std::string&kind,int index,int expected,int actual){if(expected==actual)return;if(comma)out<<',';comma=true;out<<"{\"kind\":\""<<kind<<"\",\"index\":"<<index<<",\"expected\":"<<expected<<",\"actual\":"<<actual<<'}';};
        difference("quadrants",0,g.quadrants,o.self().n_quadrants);difference("units",0,1,o.self().n_units);
        for(int i=0;i<kag::N_ITEMS;++i){difference("shed",i,g.shed[i],o.own.shed[i]);difference("carried",i,0,o.own.inv[0][i]);}
        for(int i=0;i<kag::N_CROPS;++i)difference("seeds",i,g.seeds[i],o.own.seeds[i]);
        for(int cell=0;cell<100;++cell)if(g.check[cell]){const auto actual=compositions::tile_key(o.self().tiles[cell/10][cell%10],28);for(int i=0;i<12;++i)difference("tile_field_"+std::to_string(i),cell,g.tiles[cell][i],actual[i]);}
        out<<"]}";
    }out<<"]\n";
}
