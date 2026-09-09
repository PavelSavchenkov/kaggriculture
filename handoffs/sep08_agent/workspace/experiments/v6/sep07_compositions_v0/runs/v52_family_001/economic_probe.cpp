#include "source/improved.hpp"
#include "../../include/evaluation.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include <iomanip>

using namespace compositions;
using namespace kag;

template<class Base> struct Probe {
    Base base;
    std::array<double,26> features{};
    uint64_t prefix=14695981039346656037ULL;
    template<class... Args> Probe(Args... args):base(args...){}
    void reset(const agent::AgentInit& init){base.reset(init);features={};prefix=14695981039346656037ULL;}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& a) {
        if(o.step==144) {
            features[0]=o.shops[0];features[1]=o.shops[1];
            for(int p=0;p<N_PRODUCTS;++p)features[2+p]=o.market.inventory[p];
            features[11]=o.self().money;features[12]=o.opponent().money;
            features[13]=o.opponent().n_quadrants;features[14]=o.opponent().n_units;
            for(int y=0;y<10;++y)for(int x=0;x<10;++x) {
                const auto& tile=o.opponent().tiles[y][x];
                if(tile.has_animal)++features[15+tile.what-GOOSE];
                if(tile.kind==T_PLANT)++features[18+tile.what];
            }
            features[23]=o.own.shed[WHEAT];features[24]=o.own.shed[FERTILIZER];features[25]=o.n_shops;
        }
        base.act(o,budget,a);
        if(o.step<144)hash_action(prefix,a);
    }
};
template<class Opponent> int run(const Options& o,bool force) {
    const int seats=o.seat_mode==2?2:1,count=o.seeds.size()*seats;
    std::vector<Outcome> outcomes(count);
    std::vector<std::array<double,26>> features(count);
    std::vector<uint64_t> prefixes(count);
    std::vector<int> selected(count);
    std::vector<uint32_t> matched(count);
    std::atomic<int> next{0};const auto start=std::chrono::steady_clock::now();
    auto worker=[&] {
        Probe<v52_family::OptimizedTransfer> candidate(0);
        Probe<kag::agents::late_value_s32_t0_r05::Agent> baseline;
        Opponent opponent;
        for(;;) {
            const int i=next.fetch_add(1);if(i>=count)break;
            const int seat=seats==2?i%2:o.seat_mode;
            if(force) {
                outcomes[i]=run_game(candidate,opponent,o.seeds[i/seats],seat,o);
                features[i]=candidate.features;prefixes[i]=candidate.prefix;
                selected[i]=candidate.base.selected_route();matched[i]=candidate.base.matched_days();
            } else {
                outcomes[i]=run_game(baseline,opponent,o.seeds[i/seats],seat,o);
                features[i]=baseline.features;prefixes[i]=baseline.prefix;selected[i]=-1;
            }
        }
    };
    std::vector<std::thread> pool;
    for(int i=0;i<std::min(count,o.threads);++i)pool.emplace_back(worker);
    for(auto& thread:pool)thread.join();
    write_results(o,outcomes,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    std::ofstream out(o.output+".features.csv");if(!out)std::abort();
    out<<std::setprecision(14)<<"seed,seat,prefix_hash,selected,matched";
    for(int i=0;i<26;++i)out<<",f"<<i;out<<'\n';
    for(int i=0;i<count;++i) {
        out<<outcomes[i].seed<<','<<outcomes[i].seat<<','<<prefixes[i]<<','<<selected[i]<<','<<matched[i];
        for(double value:features[i])out<<','<<value;out<<'\n';
    }
    return 0;
}
int main(int argc,char** argv) {
    const auto o=options(argc,argv);if(o.a!="force" && o.a!="baseline")return 2;
    if(o.b=="late_value_s32_t0_r05")return run<kag::agents::late_value_s32_t0_r05::Agent>(o,o.a=="force");
    if(o.b=="public_router_v52")return run<kag::agents::public_router_v52::Agent>(o,o.a=="force");
    if(o.b=="public_router")return run<public_router::Agent>(o,o.a=="force");
    return 2;
}
