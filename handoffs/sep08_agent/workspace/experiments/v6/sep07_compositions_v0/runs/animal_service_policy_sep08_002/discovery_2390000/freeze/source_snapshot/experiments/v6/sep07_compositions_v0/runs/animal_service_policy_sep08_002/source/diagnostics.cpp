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

int main(int argc,char** argv){
    const auto o=options(argc,argv);if(std::filesystem::exists(o.output))return 2;
    const int seats=o.seat_mode==2?2:1,n=o.seeds.size()*seats;
    std::vector<Outcome> results(n);std::vector<Diagnostics> diagnostics(n);std::atomic<int> next{0};
    const auto start=std::chrono::steady_clock::now();
    auto worker=[&]{
        auto own=make_agent(o.a),rival=make_agent(o.b);
        for(;;){const int i=next.fetch_add(1);if(i>=n)break;
            results[i]=run_game(own,rival,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);
            diagnostics[i]=diagnostic(o.a,own);
        }
    };
    std::vector<std::thread> threads;for(int i=0;i<std::min(n,o.threads);++i)threads.emplace_back(worker);
    for(auto& thread:threads)thread.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    std::ofstream out(o.output+".diagnostics.json");out<<"{\"games\":[";
    for(int i=0;i<n;++i){const auto& d=diagnostics[i];if(i)out<<',';
        out<<"{\"seed\":"<<results[i].seed<<",\"seat\":"<<results[i].seat<<",\"family\":"<<d.family
            <<",\"choice\":"<<d.choice<<",\"entry_step\":"<<d.entry_step<<",\"matched_days\":"<<d.matched_days
            <<",\"missed_days\":"<<d.missed_days<<",\"repaired_days\":"<<d.repaired_days
            <<",\"advanced_orders\":"<<d.advanced_orders<<",\"advanced_units\":"<<d.advanced_units;
        for(int field=0;field<3;++field){
            out<<",\""<<(field==0?"predicted_own":field==1?"predicted_margin":"deviation")<<"\":[";
            const auto& values=field==0?d.predicted_own:field==1?d.predicted_margin:d.deviation;
            for(int j=0;j<5;++j)out<<(j?",":"")<<values[j];out<<']';
        }out<<'}';
    }out<<"]}\n";
}
