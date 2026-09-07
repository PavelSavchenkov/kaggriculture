#include "../include/evaluation.hpp"
#include "../include/finance7_layer.hpp"
#include "../include/species_template.hpp"
#include "../league/destbreso_finance7/source/agent.hpp"
#include "../league/teammate_shoprouter/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

template<class Base> struct Financed {
    Base base;
    bool enabled;
    int fires=0;
    Financed(Base value,bool active):base(std::move(value)),enabled(active) {}
    void reset(const kag::agent::AgentInit& init) {base.reset(init);fires=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        base.act(o,b,a);
        if(enabled)fires+=finance7_hires(o,a);
        a.finalize();
    }
    int finance_fires() const {return fires;}
    int mirror_fires() const {return 0;}
};

template<class FactoryA,class FactoryB>
void audit(const Options& o,FactoryA make_a,FactoryB make_b,std::ofstream& events) {
    const int seats=o.seat_mode==2?2:1,count=o.seeds.size()*seats;
    std::vector<Outcome> results(count);
    std::vector<std::array<int,2>> fires(count);
    std::atomic<int> next{0};
    const auto start=std::chrono::steady_clock::now();
    auto worker=[&] {
        auto a=make_a();auto b=make_b();
        for(;;) {
            const int i=next.fetch_add(1);if(i>=count)break;
            results[i]=run_game(a,b,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);
            fires[i]={a.finance_fires(),a.mirror_fires()};
        }
    };
    std::vector<std::thread> pool;
    for(int i=0;i<std::min(o.threads,count);++i)pool.emplace_back(worker);
    for(auto& thread:pool)thread.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    for(int i=0;i<count;++i)
        events<<o.a<<','<<o.b<<','<<results[i].seed<<','<<results[i].seat<<','<<fires[i][0]<<','<<fires[i][1]<<'\n';
    events.flush();
}

int main(int argc,char** argv) {
    if(argc<2){std::cerr<<"usage: audit_finance7 DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];if(fs::exists(directory)&&!fs::is_empty(directory))return 2;
    fs::create_directories(directory);
    auto config=options(argc-1,argv+1);config.validate=true;
    std::ofstream events(directory/"layer_firings.csv");
    events<<"candidate,opponent,seed,seat,finance,mirror\n";
    auto evaluate=[&](const std::string& name,auto factory) {
        config.a=name;config.output=(directory/(name+".json")).string();
        if(config.b=="public_router")audit(config,factory,[]{return public_router::Agent{};},events);
        else if(config.b=="teammate_shoprouter")audit(config,factory,[]{return teammate_shoprouter::Agent{};},events);
        else if(config.b=="opening_router_v4")audit(config,factory,[]{return opening_router_v4::Agent{};},events);
        else if(config.b=="junghoon_78")audit(config,factory,[]{return top_replay_library::Agent(78);},events);
        else if(config.b=="finance7_base")audit(config,factory,[]{return destbreso_finance7::Agent<0>{};},events);
        else if(config.b=="pass")audit(config,factory,[]{return Pass{};},events);
        else {std::cerr<<"unsupported opponent\n";std::abort();}
    };
    for(int mode=0;mode<4;++mode)
        evaluate("finance7_mode"+std::to_string(mode),[=]{return destbreso_finance7::AgentCore(mode);});
    for(bool enabled:{false,true}) {
        const auto suffix=enabled?"_financed":"_control";
        evaluate(std::string("v4")+suffix,[=]{return Financed(opening_router_v4::Agent{},enabled);});
        for(int variant:{0,2,3,4,5})
            evaluate("species78_v"+std::to_string(variant)+suffix,[=]{return Financed(SpeciesTemplateAgent(78,variant,false),enabled);});
    }
}
