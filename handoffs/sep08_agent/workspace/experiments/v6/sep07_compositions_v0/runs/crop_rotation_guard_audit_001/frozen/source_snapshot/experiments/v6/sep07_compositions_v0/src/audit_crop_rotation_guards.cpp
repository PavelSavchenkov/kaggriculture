#include "registry.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace compositions;

bool equal_action(const kag::Action& a,const kag::Action& b) {
    if(a.n_units!=b.n_units||a.n_orders!=b.n_orders)return false;
    for(int u=0;u<a.n_units;++u)
        if(a.units[u].op!=b.units[u].op||a.units[u].arg!=b.units[u].arg||a.units[u].n!=b.units[u].n)return false;
    for(int i=0;i<a.n_orders;++i)
        if(a.orders[i].op!=b.orders[i].op||a.orders[i].item!=b.orders[i].item||a.orders[i].n!=b.orders[i].n)return false;
    return true;
}

struct AuditAgent {
    crop_rotation_t2_berry::Agent actual;
    crop_value_m2_t4::Agent base;
    crop_rotation_t2_berry::Course course;
    bool selected=false;
    int checked_actions=0;
    static kag::agent::AgentInfo info(){return {"audit_crop_rotation_guards"};}
    void reset(const kag::agent::AgentInit& init) {
        actual.reset(init);base.reset(init);course.reset(init);selected=false;checked_actions=0;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        actual.act(o,budget,action);
        kag::Action expected,alternative;base.act(o,budget,expected);course.act(o,budget,alternative);
        if(o.day==12&&o.hour==0) {
            int demand=0;
            for(int i=0;i<o.n_shops;++i)demand+=bool(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::TOMATO));
            selected=demand>=2&&(course.matched_days()&(uint32_t(1)<<12));
        }
        if(selected)expected=alternative;
        if(!equal_action(action,expected))std::abort();
        ++checked_actions;
    }
};

int main(int argc,char** argv) {
    const auto options=compositions::options(argc,argv);
    if(std::filesystem::exists(options.output))return 2;
    struct Trace {uint64_t seed,hash;int seat;double cash,rival;bool selected,berry;uint32_t matched;int actions;};
    const int count=options.seeds.size()*2;
    std::vector<Trace> traces(count);std::atomic<int> next{0};std::vector<std::thread> workers;
    for(int thread=0;thread<options.threads;++thread)workers.emplace_back([&] {
        AuditAgent own;auto rival=make_agent(options.b);
        for(int job=next.fetch_add(1);job<count;job=next.fetch_add(1)) {
            const int seat=job%2;const auto seed=options.seeds[job/2];
            const auto result=run_game(own,rival,seed,seat,options);
            traces[job]={seed,result.hash[seat],seat,result.cash[seat],result.cash[seat^1],
                         own.selected,own.course.berry_selected(),own.course.matched_days(),own.checked_actions};
        }
    });
    for(auto& worker:workers)worker.join();
    std::ofstream out(options.output);out<<"seed,seat,cash,rival_cash,action_hash,selected,berry,matched_days,checked_actions\n";
    for(const auto& t:traces)out<<t.seed<<','<<t.seat<<','<<t.cash<<','<<t.rival<<','<<t.hash<<','<<t.selected<<','<<t.berry<<','<<t.matched<<','<<t.actions<<'\n';
    std::cout<<options.b<<" traced="<<count<<'\n';
}
