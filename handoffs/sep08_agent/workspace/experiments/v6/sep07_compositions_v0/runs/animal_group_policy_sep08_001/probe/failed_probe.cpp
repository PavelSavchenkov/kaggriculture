// Offline eligibility survey. Agents receive the unchanged public observation.
#include "registry.hpp"
#include "guarded_day.hpp"
#include <filesystem>
using namespace compositions;
using namespace kag;
namespace fs=std::filesystem;
using Source=empty_sale_slots_m2::Agent;
constexpr std::array<const char*,6> names={"sheep_3","sheep_12","sheep_13","sheep_triple","cow_9","cow_triple"};

GuardedDay read_guard(const fs::path& path) {
    std::ifstream in(path);if(!in)std::abort();GuardedDay g{};
    in>>g.plan.day>>g.quadrants;
    for(int c=0;c<100;++c){bool check;in>>check;g.check[c]=check;for(auto& x:g.tiles[c])in>>x;}
    for(auto& x:g.shed)in>>x;for(auto& x:g.seeds)in>>x;
    if(!in)std::abort();return g;
}
struct Entry {
    bool exact=false;
    int tile_mismatches=0,stock_mismatches=0,frame_mismatches=0;
    int cash=0,nshops=0,cows=0,sheep=0,geese=0;
    std::array<int,N_ITEMS> demand{};
};
Entry inspect(const GuardedDay& g,const agent::AgentObservation& o) {
    Entry e;e.exact=g.matches(o);e.cash=o.self().money;e.nshops=o.n_shops;
    e.frame_mismatches=(o.self().n_units!=1)+(o.self().n_quadrants!=g.quadrants)+
        (o.self().pos_x[0]!=4)+(o.self().pos_y[0]!=4);
    for(int i=0;i<N_ITEMS;++i)e.stock_mismatches+=(o.own.shed[i]!=g.shed[i])+(o.own.inv[0][i]!=0);
    for(int i=0;i<N_CROPS;++i)e.stock_mismatches+=o.own.seeds[i]!=g.seeds[i];
    for(int c=0;c<100;++c){
        const auto& tile=o.self().tiles[c/10][c%10];
        e.tile_mismatches+=g.check[c] && tile_key(tile,o.day)!=g.tiles[c];
        e.cows+=tile.has_animal && tile.what==COW;e.sheep+=tile.has_animal && tile.what==SHEEP;
        e.geese+=tile.has_animal && tile.what==GOOSE;
    }
    for(int s=0;s<o.n_shops;++s)for(int i=0;i<N_ITEMS;++i)e.demand[i]+=SHOP_MULT[o.shops[s]][i];
    return e;
}
struct Record {
    Outcome outcome;
    std::array<Entry,6> entry;
    std::array<uint64_t,30> unit_hash{};
    int berry20=0;
};
Record run(uint64_t seed,int seat,const Options& options,const std::array<GuardedDay,6>& guards) {
    Config config;config.seed=seed;Sim sim(config);Source own;auto rival=make_agent(options.b);
    own.reset(agent::runtime::make_agent_init(sim,seat));rival.reset(agent::runtime::make_agent_init(sim,seat^1));
    agent::DecisionBudget budget;budget.max_expansions=options.expansions;
    Record r;r.outcome.seed=seed;r.outcome.seat=seat;r.unit_hash.fill(14695981039346656037ULL);
    uint64_t rng=seed^0xa37108e62d045fb9ULL;for(auto& s:r.outcome.shops)s=random_word(rng)%N_SHOPS;
    while(!sim.st.done) {
        if(!options.native_shops)std::copy_n(r.outcome.shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto a=agent::runtime::make_observation(sim,seat),b=agent::runtime::make_observation(sim,seat^1);
        if(a.hour==0){
            for(int i=0;i<6;++i)if(a.day==guards[i].plan.day)r.entry[i]=inspect(guards[i],a);
            if(a.day==20)for(int s=0;s<a.n_shops;++s)r.berry20+=SHOP_MULT[a.shops[s]][STRAWBERRY];
        }
        Action acts[2];own.act(a,budget,acts[seat]);rival.act(b,budget,acts[seat^1]);
        validate_action(acts[seat],a);validate_action(acts[seat^1],b);
        for(int p=0;p<2;++p)hash_action(r.outcome.hash[p],acts[p]);
        auto units=acts[seat];units.n_orders=0;units.finalize();hash_action(r.unit_hash[a.day],units);
        sim.step(acts[0],acts[1]);
    }
    if(sim.st.step!=719)std::abort();r.outcome.turns=sim.st.step;
    for(int p=0;p<2;++p)r.outcome.cash[p]=sim.st.farms[p].money;
    if(options.native_shops)std::copy_n(sim.st.shops,8,r.outcome.shops.begin());
    return r;
}
int main(int argc,char** argv) {
    auto o=options(argc,argv);if(fs::exists(o.output))return 2;
    const fs::path root="experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/integrated_30s";
    std::array<GuardedDay,6> guards;
    for(int i=0;i<6;++i)guards[i]=read_guard(root/names[i]/"days"/(i<4?"20":"15")/"guard.txt");
    const int seats=o.seat_mode==2?2:1,n=o.seeds.size()*seats;
    std::vector<Record> records(n);std::atomic<int> next{0};
    auto worker=[&]{for(;;){const int i=next.fetch_add(1);if(i>=n)break;records[i]=run(o.seeds[i/seats],seats==2?i%2:o.seat_mode,o,guards);}};
    std::vector<std::thread> threads;for(int i=0;i<std::min(n,o.threads);++i)threads.emplace_back(worker);
    for(auto& t:threads)t.join();
    // Independent standard-runner controls cover every surveyed opponent/seat.
    for(int i=0;i<std::min(n,8);++i){
        Source a;auto b=make_agent(o.b);const auto& r=records[i].outcome;
        const auto c=run_game(a,b,r.seed,r.seat,o);
        for(int p=0;p<2;++p)if(c.cash[p]!=r.cash[p] || c.hash[p]!=r.hash[p])std::abort();
    }
    std::ofstream out(o.output);if(!out)std::abort();
    out<<"{\"opponent\":\""<<o.b<<"\",\"controls\":"<<std::min(n,8)<<",\"records\":[";
    for(int k=0;k<n;++k){const auto& r=records[k];if(k)out<<',';
        out<<"{\"seed\":"<<r.outcome.seed<<",\"seat\":"<<r.outcome.seat<<",\"berry20\":"<<r.berry20<<",\"days\":[";
        for(int d=0;d<30;++d)out<<(d?",":"")<<'"'<<r.unit_hash[d]<<'"';
        out<<"],\"entries\":[";
        for(int i=0;i<6;++i){const auto& e=r.entry[i];if(i)out<<',';
            out<<"{\"name\":\""<<names[i]<<"\",\"exact\":"<<e.exact<<",\"tiles\":"<<e.tile_mismatches
                <<",\"stock\":"<<e.stock_mismatches<<",\"frame\":"<<e.frame_mismatches<<",\"cash\":"<<e.cash
                <<",\"shops\":"<<e.nshops<<",\"herd\":["<<e.cows<<','<<e.sheep<<','<<e.geese<<"],\"demand\":[";
            for(int j=0;j<N_ITEMS;++j)out<<(j?",":"")<<e.demand[j];out<<"]}";
        }out<<"]}";
    }out<<"]}\n";std::cout<<o.b<<" complete "<<n<<" games\n";
}
