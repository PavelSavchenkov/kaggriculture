#include "../../include/evaluation.hpp"
#include "../../include/guarded_day.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include "../late_portfolio_001/proposals/late_value_s32_t0_r05/source/agent.hpp"
#include <iomanip>
#include <sstream>

using namespace compositions;
using namespace kag;

struct Snapshot {
    int step,route;double cash;
    std::vector<int> physical;
    std::array<int,3> animals{};
};
template<class Base> struct Trace {
    Base base;
    std::vector<Snapshot> states;
    std::vector<std::array<int,5>> markets;
    void reset(const agent::AgentInit& init){base.reset(init);states.clear();markets.clear();}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& action) {
        base.act(o,budget,action);
        if(o.hour==0 || o.step==1) {
            Snapshot s;s.step=o.step;s.cash=o.self().money;s.route=-1;
            if constexpr(requires{base.selected_route();})s.route=base.selected_route();
            const auto& f=o.self();
            auto add=[&](int x){s.physical.push_back(x);};
            add(f.n_units);add(f.n_quadrants);add(f.hires_today);
            for(int i=0;i<N_ITEMS;++i)add(o.own.shed[i]);
            for(int i=0;i<N_CROPS;++i)add(o.own.seeds[i]);
            for(int i=0;i<f.n_units;++i) {
                add(f.pos_x[i]);add(f.pos_y[i]);
                for(int p=0;p<N_ITEMS;++p)add(o.own.inv[i][p]);
                add(o.own.inv_nkeys[i]);
                for(int k=0;k<o.own.inv_nkeys[i];++k)add(o.own.inv_keys[i][k]);
            }
            for(int y=0;y<10;++y)for(int x=0;x<10;++x) {
                const auto& t=f.tiles[y][x];
                for(int v:tile_key(t,o.day))add(v);
                if(t.has_animal)++s.animals[t.what-GOOSE];
            }
            states.push_back(std::move(s));
        }
        for(int i=0;i<action.n_orders;++i) {
            const auto& a=action.orders[i];
            markets.push_back({o.step,i,a.op,a.item,a.n});
        }
    }
};
template<class T> void array(std::ostream& out,const T& values) {
    out<<'[';bool first=true;for(const auto v:values){if(!first)out<<',';out<<v;first=false;}out<<']';
}
template<class T> void write(std::ostream& out,const Trace<T>& trace) {
    out<<"{\"states\":[";
    for(size_t i=0;i<trace.states.size();++i) {
        const auto& s=trace.states[i];if(i)out<<',';
        out<<"{\"step\":"<<s.step<<",\"route\":"<<s.route<<",\"cash\":"<<s.cash<<",\"animals\":";
        array(out,s.animals);out<<",\"physical\":";array(out,s.physical);out<<'}';
    }
    out<<"],\"market_actions\":[";
    for(size_t i=0;i<trace.markets.size();++i){if(i)out<<',';array(out,trace.markets[i]);}
    out<<"]}";
}
int main(int argc,char** argv) {
    const auto o=options(argc,argv);
    const int seats=o.seat_mode==2?2:1,count=o.seeds.size()*seats;
    std::vector<Outcome> outcomes(count);std::vector<std::string> traces(count);
    std::atomic<int> next{0};
    auto worker=[&] {
        Trace<kag::agents::public_router_v52::Agent> own;
        Trace<kag::agents::late_value_s32_t0_r05::Agent> other;
        for(;;) {
            const int i=next.fetch_add(1);if(i>=count)break;
            outcomes[i]=run_game(own,other,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);
            std::ostringstream out;out<<std::setprecision(12)<<"{\"seed\":"<<outcomes[i].seed<<",\"seat\":"<<outcomes[i].seat<<",\"v52\":";
            write(out,own);out<<",\"portfolio\":";write(out,other);out<<'}';traces[i]=out.str();
        }
    };
    const auto start=std::chrono::steady_clock::now();
    std::vector<std::thread> pool;for(int i=0;i<std::min(count,o.threads);++i)pool.emplace_back(worker);
    for(auto& thread:pool)thread.join();
    write_results(o,outcomes,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    std::ofstream out(o.output+".traces.jsonl");if(!out)std::abort();
    for(const auto& trace:traces)out<<trace<<'\n';
    return 0;
}
