#include "../v52_family_001/source/improved.hpp"
#include "../../include/evaluation.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include "../../candidates/junghoon_wool_sales/source/agent.hpp"
#include <iomanip>

using namespace compositions;
using namespace kag;

template<class Base> struct Probe {
    Base base;
    std::array<double,56> features{};
    uint64_t prefix=14695981039346656037ULL;
    template<class... Args> Probe(Args... args):base(args...){}
    void reset(const agent::AgentInit& init){base.reset(init);features={};prefix=14695981039346656037ULL;}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& a) {
        if(o.step>=144 && o.step<=150) {
            const int k=(o.step-144)*8;
            features[k]=o.opponent().n_units;features[k+1]=o.opponent().money;
            features[k+2]=o.self().n_units;features[k+3]=o.self().money;
            features[k+4]=o.market.inventory[WHEAT];features[k+5]=o.market.inventory[FERTILIZER];
            uint64_t h=14695981039346656037ULL;
            auto add=[&](int n){h^=uint64_t(n);h*=1099511628211ULL;};
            const auto& f=o.opponent();add(f.n_quadrants);add(f.n_units);
            for(int u=0;u<f.n_units;++u){add(f.pos_x[u]);add(f.pos_y[u]);}
            for(int y=0;y<10;++y)for(int x=0;x<10;++x)
                for(int v:tile_key(f.tiles[y][x],o.day))add(v);
            features[k+6]=uint32_t(h);features[k+7]=uint32_t(h>>32);
        }
        base.act(o,budget,a);
        if(o.step<144)hash_action(prefix,a);
    }
};
template<class Opponent> int run(const Options& o,bool force) {
    const int seats=o.seat_mode==2?2:1,count=o.seeds.size()*seats;
    std::vector<Outcome> outcomes(count);
    std::vector<std::array<double,56>> features(count);
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
    for(int i=0;i<56;++i)out<<",f"<<i;out<<'\n';
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
    if(o.b=="junghoon_wool_sales")return run<junghoon_wool_sales::Agent>(o,o.a=="force");
    return 2;
}
