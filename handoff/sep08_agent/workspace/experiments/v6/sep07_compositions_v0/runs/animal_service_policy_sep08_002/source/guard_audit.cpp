// Ordinary registered policies plus offline diagnostics after each game.
#include "registry.hpp"
#include <filesystem>
using namespace compositions;
using Diagnostics=animal_repair::Diagnostics;
Diagnostics convert(const auto& from){
    Diagnostics to;
    to.family=from.family;
    to.choice=from.choice;
    to.entry_step=from.entry_step;
    to.advanced_orders=from.advanced_orders;
    to.advanced_units=from.advanced_units;
    to.matched_days=from.matched_days;
    to.missed_days=from.missed_days;
    to.repaired_days=from.repaired_days;
    to.predicted_own=from.predicted_own;
    to.predicted_margin=from.predicted_margin;
    to.deviation=from.deviation;
    return to;
}
Diagnostics diagnostic(const std::string& name,AgentBox& box){
    if(name=="animal_repair_q24_premium_m2")return convert(static_cast<AgentModel<animal_repair_q24_premium_m2::Agent>*>(box.value.get())->value.diagnostics());
    if(name=="cow_service_retained_q24_premium_m2")return convert(static_cast<AgentModel<cow_service_retained_q24_premium_m2::Agent>*>(box.value.get())->value.diagnostics());
    if(name=="cow_service_retained_fixed_choices_m2")return convert(static_cast<AgentModel<cow_service_retained_fixed_choices_m2::Agent>*>(box.value.get())->value.diagnostics());
    std::abort();
}

#include <sstream>
struct Inspector {
    AgentBox policy;
    std::string name;
    int leaf=0;
    std::vector<std::string> differences;
    static kag::agent::AgentInfo info(){return {"guard_audit"};}
    void reset(const kag::agent::AgentInit& init){policy.reset(init);leaf=0;differences.clear();}
    Diagnostics diagnostics(){return diagnostic(name,policy);}
    static int distance(const GuardedDay& g,const kag::agent::AgentObservation& o){
        const auto& f=o.self();int n=1000*(f.n_units!=1 || f.n_quadrants!=g.quadrants || f.pos_x[0]!=4 || f.pos_y[0]!=4);
        for(int i=0;i<kag::N_ITEMS;++i)n+=(o.own.shed[i]!=g.shed[i])+(o.own.inv[0][i]!=0);
        for(int i=0;i<kag::N_CROPS;++i)n+=o.own.seeds[i]!=g.seeds[i];
        for(int c=0;c<100;++c)n+=10*(g.check[c] && tile_key(f.tiles[c/10][c%10],o.day)!=g.tiles[c]);
        return n;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        policy.act(o,b,a);if(o.hour)return;const auto d=diagnostics();if(d.family<0)return;
        const auto& family=(name=="animal_repair_q24_premium_m2"?animal_groups_policy::library():cow_service_retained::library())[d.family];const int index=o.day-family.first;
        if(d.family==1 && d.entry_step==o.step){int demand=0;for(int s=0;s<o.n_shops;++s)
            if(kag::SHOP_MASK[o.shops[s]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[s]];leaf=demand>=4;}
        if(d.family==0)leaf=distance(family.choices[d.choice][1].days[index],o)<distance(family.choices[d.choice][0].days[index],o);
        const GuardedDay* plan=&family.choices[d.choice][leaf].days[index];
        if(d.repaired_days&(uint32_t{1}<<o.day))plan=&animal_repair::repair_day();
        const auto& g=*plan;const bool missed=d.missed_days&(uint32_t{1}<<o.day);
        if(missed==g.matches(o))std::abort();if(!missed)return;
        auto record=[&](const char* kind,int item,int field,int expected,int actual){
            if(expected==actual)return;std::ostringstream row;
            row<<o.day<<','<<d.family<<','<<d.choice<<','<<leaf<<','<<kind<<','<<item<<','<<field<<','<<expected<<','<<actual;
            differences.push_back(row.str());
        };
        const auto& f=o.self();record("farm",0,0,1,f.n_units);record("farm",0,1,g.quadrants,f.n_quadrants);
        record("farm",0,2,4,f.pos_x[0]);record("farm",0,3,4,f.pos_y[0]);
        for(int i=0;i<kag::N_ITEMS;++i){record("shed",i,0,g.shed[i],o.own.shed[i]);record("carry",i,0,0,o.own.inv[0][i]);}
        for(int i=0;i<kag::N_CROPS;++i)record("seed",i,0,g.seeds[i],o.own.seeds[i]);
        for(int c=0;c<100;++c)if(g.check[c]){const auto actual=tile_key(f.tiles[c/10][c%10],o.day);
            for(int field=0;field<12;++field)record("tile",c,field,g.tiles[c][field],actual[field]);}
    }
};

int main(int argc,char** argv){
    const auto o=options(argc,argv);if(std::filesystem::exists(o.output))return 2;
    const int seats=o.seat_mode==2?2:1,n=o.seeds.size()*seats;
    std::vector<Outcome> results(n);std::vector<std::vector<std::string>> differences(n);std::atomic<int> next{0};
    const auto start=std::chrono::steady_clock::now();
    auto worker=[&]{Inspector own{make_agent(o.a),o.a};auto rival=make_agent(o.b);
        for(;;){const int i=next.fetch_add(1);if(i>=n)break;
            results[i]=run_game(own,rival,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);differences[i]=own.differences;}};
    std::vector<std::thread> threads;for(int i=0;i<std::min(n,o.threads);++i)threads.emplace_back(worker);
    for(auto& thread:threads)thread.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    std::ofstream out(o.output+".differences.csv");out<<"seed,seat,day,family,choice,leaf,kind,item,field,expected,actual\n";
    for(int i=0;i<n;++i)for(const auto& row:differences[i])out<<results[i].seed<<','<<results[i].seat<<','<<row<<'\n';
}
