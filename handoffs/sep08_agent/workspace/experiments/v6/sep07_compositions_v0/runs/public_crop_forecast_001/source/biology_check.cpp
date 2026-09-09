// Offline single-tile service diagnostic, not a gameplay policy. Funding and
// three colocated workers are supplied explicitly to isolate crop biology.
#include "crop_forecast.hpp"
#include <fstream>
#include <iostream>

int main(int argc,char**argv){
    if(argc!=3)return 2;
    using namespace compositions::public_crop_forecast;
    std::ifstream input(argv[1]);std::ofstream out(argv[2]);int count;input>>count;
    long tiles=0,checks=0,errors=0;out<<"{\"mismatches\":[";
    for(int c=0;c<count;++c){
        kag::agent::AgentObservation observed{};int n;input>>observed.step>>observed.day>>observed.hour>>n;
        for(int i=0;i<n;++i){
            int x,y,kind,what,animal,water,fed,care,fert,dry,yield,bonus,planted,maxlife,until;
            input>>x>>y>>kind>>what>>animal>>water>>fed>>care>>fert>>dry>>yield>>bonus>>planted>>maxlife>>until;
            if(kind!=kag::T_PLANT)continue;++tiles;
            kag::Tile initial{};initial.kind=kag::T_PLANT;initial.what=what;initial.watered_today=water;
            initial.consecutive_dry=dry;initial.yield_units=yield;initial.planted_day=planted;
            initial.max_lifespan_step=maxlife;initial.fertilized_until_day=until;
            for(int mode=1;mode<=5;++mode){
                auto o=observed;o.farms[1].tiles[0][0]=initial;const auto predicted=crop_output(o,Mode(mode));
                kag::Config cfg;cfg.weed_chance=0;kag::Sim sim(cfg);
                sim.st.step=o.step;sim.st.day=o.day;sim.st.hour=o.hour;
                auto&farm=sim.st.farms[1];farm.tiles[0][0]=initial;
                const bool maintain=mode==4||(mode==3&&until>=o.day);
                Days actual{};
                while(!sim.st.done){
                    const int day=sim.st.day;auto&t=farm.tiles[0][0];if(t.kind!=kag::T_PLANT)break;
                    farm.n_units=3;
                    for(int u=0;u<3;++u){farm.pos_x[u]=farm.pos_y[u]=0;farm.inv_clear(u);}
                    farm.inv_add(0,kag::FERTILIZER,1);
                    kag::Action pass,a;a.n_units=3;
                    a.units[0].op=maintain&&t.fertilized_until_day<day?kag::OP_FERTILIZE:kag::OP_PASS;
                    a.units[1].op=kag::OP_WATER;
                    const auto&crop=kag::CROPS[what];const int age=day-planted;
                    const bool ripe=age>=crop.first_yield_day;
                    const bool harvest=crop.ongoing || day>=std::min(29,planted+crop.max_yield_day) ||
                        mode==5 || (mode!=1 && t.yield_units>=crop.max_yield);
                    a.units[2].op=ripe&&harvest?kag::OP_HARVEST:kag::OP_PASS;
                    const auto before=farm.produced[what];sim.step(pass,a);actual[day][what]+=farm.produced[what]-before;
                }
                ++checks;bool mismatch=false;
                for(int day=0;day<30;++day)mismatch|=predicted.output[day][what]!=actual[day][what];
                if(mismatch){if(errors++)out<<',';out<<"{\"case\":"<<c<<",\"x\":"<<x<<",\"y\":"<<y<<",\"crop\":"<<what<<",\"mode\":"<<mode<<",\"predicted\":[";
                    for(int day=0;day<30;++day){if(day)out<<',';out<<predicted.output[day][what];}out<<"],\"actual\":[";
                    for(int day=0;day<30;++day){if(day)out<<',';out<<actual[day][what];}out<<"]}";}
            }
        }
    }
    out<<"],\"tiles\":"<<tiles<<",\"checks\":"<<checks<<",\"errors\":"<<errors<<"}\n";
    std::cout<<"tiles="<<tiles<<" checks="<<checks<<" errors="<<errors<<'\n';
}
