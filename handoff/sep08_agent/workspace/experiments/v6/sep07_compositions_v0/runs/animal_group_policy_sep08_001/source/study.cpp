// Offline telemetry. Runtime policies receive only ordinary observations.
#include "registry.hpp"
#include "policy.hpp"
#include <filesystem>
using namespace compositions;
using namespace animal_groups_policy;
int main(int argc,char** argv){
    const auto o=options(argc,argv);if(std::filesystem::exists(o.output))return 2;
    int mode=-1,family=-1,choice=-1;
    if(o.a=="animal_groups_m0")mode=0;
    else if(o.a=="animal_groups_m1")mode=1;
    else if(o.a=="animal_groups_m2")mode=2;
    else if(o.a=="animal_groups_cow1"){mode=1;family=0;choice=1;}
    else if(o.a=="animal_groups_cow3"){mode=1;family=0;choice=2;}
    else if(o.a=="animal_groups_sheep12"){mode=1;family=1;choice=2;}
    else if(o.a=="animal_groups_sheep3"){mode=1;family=1;choice=4;}
    else return 2;
    const int seats=o.seat_mode==2?2:1,n=o.seeds.size()*seats;
    std::vector<Outcome> results(n);std::vector<Diagnostics> diagnostics(n);std::atomic<int> next{0};
    const auto start=std::chrono::steady_clock::now();
    auto worker=[&]{
        Policy own(mode,family,choice);auto rival=make_agent(o.b);
        for(;;){const int i=next.fetch_add(1);if(i>=n)break;
            results[i]=run_game(own,rival,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);
            diagnostics[i]=own.diagnostics();
        }
    };
    std::vector<std::thread> threads;for(int i=0;i<std::min(n,o.threads);++i)threads.emplace_back(worker);
    for(auto& t:threads)t.join();
    // Standard registered adapters must reproduce the instrumented driver.
    for(int i=0;i<std::min(n,8);++i){
        auto a=make_agent(o.a),b=make_agent(o.b);const auto& r=results[i];
        const auto control=run_game(a,b,r.seed,r.seat,o);
        for(int p=0;p<2;++p)if(control.cash[p]!=r.cash[p] || control.hash[p]!=r.hash[p])std::abort();
    }
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    std::ofstream out(o.output+".diagnostics.json");
    out<<"{\"controls\":"<<std::min(n,8)<<",\"games\":[";
    for(int i=0;i<n;++i){const auto& d=diagnostics[i];if(i)out<<',';
        out<<"{\"seed\":"<<results[i].seed<<",\"seat\":"<<results[i].seat<<",\"family\":"<<d.family
            <<",\"choice\":"<<d.choice<<",\"entry_step\":"<<d.entry_step<<",\"matched_days\":"<<d.matched_days
            <<",\"missed_days\":"<<d.missed_days<<",\"advanced_orders\":"<<d.advanced_orders<<",\"advanced_units\":"<<d.advanced_units;
        for(int field=0;field<3;++field){
            out<<",\""<<(field==0?"predicted_own":field==1?"predicted_margin":"deviation")<<"\":[";
            const auto& values=field==0?d.predicted_own:field==1?d.predicted_margin:d.deviation;
            for(int j=0;j<5;++j)out<<(j?",":"")<<values[j];out<<']';
        }out<<'}';
    }out<<"]}\n";
}
