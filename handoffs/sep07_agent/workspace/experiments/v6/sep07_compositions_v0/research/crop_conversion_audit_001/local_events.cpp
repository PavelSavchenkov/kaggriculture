#include "evaluation.hpp"
#include "registry.hpp"
int main(int argc,char**argv){
    const auto o=compositions::options(argc,argv);std::ofstream out(o.output);out<<'[';bool comma=false;
    for(uint64_t seed:o.seeds)for(int seat=0;seat<2;++seat){kag::Config config;config.seed=seed;kag::Sim sim(config);auto a=make_agent("investment_context_guarded_001_best"),b=make_agent("public_router");
        a.reset(kag::agent::runtime::make_agent_init(sim,seat));b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));kag::agent::DecisionBudget budget;budget.max_expansions=100000;
        std::array<uint8_t,8>shops{};uint64_t rng=seed^0xa37108e62d045fb9ULL;for(auto&shop:shops)shop=compositions::random_word(rng)%kag::N_SHOPS;
        uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
        if(comma)out<<',';comma=true;out<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"events\":[";bool event_comma=false;
        while(!sim.st.done){for(int i=0;i<8;++i)if(i<sim.st.n_shops)sim.st.shops[i]=shops[i];
            const auto oa=kag::agent::runtime::make_observation(sim,seat),ob=kag::agent::runtime::make_observation(sim,seat^1);kag::Action acts[2];a.act(oa,budget,acts[seat]);b.act(ob,budget,acts[seat^1]);
            compositions::validate_action(acts[seat],oa);compositions::validate_action(acts[seat^1],ob);for(int p=0;p<2;++p)compositions::hash_action(hashes[p],acts[p]);
            auto unit_only=acts[seat];unit_only.n_orders=0;const auto accepted=sim.sanitize_solo_action(seat,unit_only);kag::Action prefix[2];
            for(int p=0;p<2;++p){prefix[p].clear();prefix[p].n_units=sim.st.farms[p].n_units;prefix[p].finalize();}
            auto previous=sim;previous.st.hour=0;previous.step(prefix[0],prefix[1]);
            for(int u=0;u<accepted.n_units;++u){prefix[seat].units[u]=accepted.units[u];prefix[seat].finalize();auto next=sim;next.st.hour=0;next.step(prefix[0],prefix[1]);
                const auto command=accepted.units[u];const auto&farm=sim.st.farms[seat];const int x=farm.pos_x[u],y=farm.pos_y[u];
                const auto&old_tile=previous.st.farms[seat].tiles[y][x];const auto&initial_tile=farm.tiles[y][x];const auto&tile=old_tile.kind==kag::T_PLANT?old_tile:initial_tile;
                int item=command.op==kag::OP_PLANT?command.arg:tile.kind==kag::T_PLANT?tile.what:-1;
                if(item>=0&&item<5&&(command.op==kag::OP_PLANT||command.op==kag::OP_WATER||command.op==kag::OP_HARVEST||command.op==kag::OP_FERTILIZE||command.op==kag::OP_DIG)){
                    const int quantity=command.op==kag::OP_HARVEST?next.st.farms[seat].produced[item]-previous.st.farms[seat].produced[item]:command.op==kag::OP_FERTILIZE;
                    if(event_comma)out<<',';event_comma=true;out<<'['<<sim.st.step<<','<<u<<','<<int(command.op)<<','<<item<<','<<x<<','<<y<<','<<(command.op==kag::OP_PLANT?sim.st.day:tile.planted_day)<<','<<quantity<<','<<tile.fertilized_until_day<<']';
                }previous=next;
            }sim.step(acts[0],acts[1]);
        }out<<"],\"cash\":"<<sim.st.farms[seat].money<<",\"opponent_cash\":"<<sim.st.farms[seat^1].money<<",\"action_hash\":"<<hashes[seat]<<",\"opponent_action_hash\":"<<hashes[seat^1]<<'}';
    }out<<"]\n";
}
