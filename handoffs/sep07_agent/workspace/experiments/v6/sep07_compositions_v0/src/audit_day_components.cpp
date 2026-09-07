#include "../include/evaluation.hpp"
#include "../candidates/combine_hires_001_best/source/agent.hpp"
#include "../runs/reduce_hires_001/proposals/reduce_hires_001_23/source/agent.hpp"
#include "../league/mao_85/source/agent.hpp"
#include "../league/junghoon_78/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

class DayComponentAgent {
    combine_hires_001_best::Agent parent_;
    std::array<kag::Action,24> day_=reduce_hires_001_23::schedule();
    int mode_;
    bool selected_=false;
public:
    explicit DayComponentAgent(int mode):mode_(mode) {}
    static kag::agent::AgentInfo info() {return {"day_component_audit"};}
    void reset(const kag::agent::AgentInit& init) {parent_.reset(init);selected_=false;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        parent_.act(o,budget,action);
        if(o.step==1)selected_=o.opponent().n_units==1;
        if(!selected_ || o.day!=1 || mode_==0)return;
        const auto original=action;auto compiled=day_[o.hour];
        for(int u=compiled.n_units;u<o.self().n_units;++u)compiled.units[u]={};
        compiled.n_units=o.self().n_units;
        if(mode_==1)action=compiled;
        if(mode_==2 || mode_==5) {
            action=compiled;action.n_orders=original.n_orders;
            std::copy_n(original.orders,original.n_orders,action.orders);
        }
        if(mode_==3) {
            action.n_orders=std::max(original.n_orders,compiled.n_orders);
            std::copy_n(compiled.orders,compiled.n_orders,action.orders);
            for(int i=0;i<original.n_orders;++i)
                if(original.orders[i].op==kag::M_HIRE)action.orders[i]=original.orders[i];
        }
        if(mode_==4 || mode_==5) {
            for(int i=0;i<original.n_orders;++i)
                if(original.orders[i].op==kag::M_HIRE && (i>=compiled.n_orders || compiled.orders[i].op!=kag::M_HIRE))action.orders[i]={};
        }
        action.finalize();
    }
};

int main(int argc,char** argv) {
    if(argc<2) {std::cerr<<"usage: audit_day_components DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];
    if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);
    const char* names[]={"parent","full_day1","routes_keep_hire","accepted_markets_keep_hire","delete_hire_only","routes_delete_hire"};
    for(int mode=0;mode<6;++mode) {
        config.a=names[mode];
        auto own=[mode]{return DayComponentAgent(mode);};
        auto run=[&](const char* name,auto opponent) {
            config.b=name;config.output=(directory/(std::string(names[mode])+"_vs_"+name+".json")).string();
            run_batch(config,own,opponent);
        };
        run("opening_router_v2",[]{return opening_router_v2::Agent{};});
        run("public_router",[]{return public_router::Agent{};});
        run("mao_85",[]{return mao_85::Agent{};});
        run("junghoon_78",[]{return junghoon_78::Agent{};});
    }
}
