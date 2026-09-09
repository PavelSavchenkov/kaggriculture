#include "../rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"
#include "../../league/public_router_v52/source/agent.hpp"
#include "../../include/evaluation.hpp"
#include <iomanip>
#include <sstream>

using namespace compositions;
using namespace kag;

template<class Range> void array(std::ostream& out,const Range& values) {
    out<<'[';bool first=true;
    for(auto value:values){if(!first)out<<',';out<<int(value);first=false;}
    out<<']';
}

template<class Agent> void trace_step(const Sim& sim,const agent::AgentObservation& o,
        const Action (&actions)[2],const Agent& policy,std::ostream& out) {
    const auto joint=sim.diagnose_joint_actions(actions[0],actions[1]);
    const auto& d=joint.players[o.player];
    const bool faults=d.requested_unit_actions!=d.successful_unit_actions;
    const bool orders=d.requested_order_units!=d.successful_order_units;
    if(o.hour!=0 && !faults && !orders && o.step!=145)return;
    out<<"{\"step\":"<<o.step<<",\"cash\":"<<o.self().money<<",\"units\":"<<o.self().n_units
       <<",\"selected\":"<<policy.selected()<<",\"matched\":"<<policy.matched_days()
       <<",\"unit_faults\":"<<d.requested_unit_actions-d.successful_unit_actions
       <<",\"failed_order_units\":"<<d.requested_order_units-d.successful_order_units<<",\"shed\":";
    array(out,o.own.shed);out<<",\"seeds\":";array(out,o.own.seeds);
    out<<",\"market_prices\":";array(out,o.market.prices);out<<",\"orders\":[";
    const auto& a=actions[o.player];
    for(int i=0;i<a.n_orders;++i){if(i)out<<',';const auto& v=a.orders[i];array(out,std::array<int,3>{v.op,v.item,v.n});}
    auto solo=a;solo.n_orders=0;
    const auto accepted=sim.sanitize_solo_action(o.player,solo);
    out<<"],\"failed_units\":[";int failed=0;
    for(int u=0;u<a.n_units;++u)if(a.units[u].op!=OP_PASS && accepted.units[u].op==OP_PASS) {
        if(failed++)out<<',';
        const auto& v=a.units[u];const int x=o.self().pos_x[u],y=o.self().pos_y[u];
        out<<"{\"unit\":"<<u<<",\"x\":"<<x<<",\"y\":"<<y<<",\"action\":";
        array(out,std::array<int,3>{v.op,v.arg,v.n});out<<",\"inventory\":";array(out,o.own.inv[u]);
        out<<",\"tile\":";array(out,tile_key(o.self().tiles[y][x],o.day));out<<'}';
    }
    if(failed!=d.requested_unit_actions-d.successful_unit_actions)std::abort();
    out<<"],\"day_guard_differences\":[";bool first=true;
    if(o.hour==0 && policy.selected())for(const auto& g:v52_family::improved_days())if(g.plan.day==o.day) {
        if(!first)out<<',';first=false;out<<"{\"matches\":"<<g.matches(o)<<",\"shed_delta\":[";
        for(int i=0;i<N_ITEMS;++i){if(i)out<<',';out<<o.own.shed[i]-g.shed[i];}
        out<<"],\"seed_delta\":[";for(int i=0;i<N_CROPS;++i){if(i)out<<',';out<<o.own.seeds[i]-g.seeds[i];}
        out<<"],\"tiles\":[";bool cell_first=true;
        for(int cell=0;cell<100;++cell)if(g.check[cell] && tile_key(o.self().tiles[cell/10][cell%10],o.day)!=g.tiles[cell]) {
            if(!cell_first)out<<',';cell_first=false;
            out<<"{\"cell\":"<<cell<<",\"actual\":";array(out,tile_key(o.self().tiles[cell/10][cell%10],o.day));
            out<<",\"expected\":";array(out,g.tiles[cell]);out<<'}';
        }
        out<<"]}";
    }
    out<<"]}\n";
}

#include "traced_game.inc"

int main(int argc,char** argv) {
    const auto o=options(argc,argv);
    const int seats=o.seat_mode==2?2:1,count=o.seeds.size()*seats;
    std::vector<Outcome> results(count);std::vector<std::string> traces(count);std::atomic<int> next{0};
    const auto start=std::chrono::steady_clock::now();
    auto worker=[&] {
        kag::agents::rival_wool_context_v3::Agent own;
        kag::agents::public_router_v52::Agent rival;
        for(;;) {
            const int i=next.fetch_add(1);if(i>=count)break;
            std::ostringstream out;out<<std::setprecision(14);
            results[i]=traced_game(own,rival,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o,out);
            traces[i]=out.str();
        }
    };
    std::vector<std::thread> pool;for(int i=0;i<std::min(count,o.threads);++i)pool.emplace_back(worker);
    for(auto& thread:pool)thread.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    for(int i=0;i<count;++i){std::ofstream out(o.output+"."+std::to_string(results[i].seed)+"_"+std::to_string(results[i].seat)+".trace.jsonl");out<<traces[i];}
}
