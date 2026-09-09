#include "crop_forecast.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>

int main(int argc,char**argv){
    if(argc!=3)return 2;std::ifstream input(argv[1]);std::ofstream output(argv[2]);output<<std::setprecision(17)<<'[';
    int count;input>>count;
    for(int c=0;c<count;++c){
        kag::agent::AgentObservation o{};o.player=0;int n;input>>o.step>>o.day>>o.hour>>n;
        for(int i=0;i<n;++i){int x,y,kind,what,animal,water,fed,care,fert,dry,yield,bonus,planted,maxlife,until;input>>x>>y>>kind>>what>>animal>>water>>fed>>care>>fert>>dry>>yield>>bonus>>planted>>maxlife>>until;
            auto&t=o.farms[1].tiles[y][x];t.kind=kag::TileKind(kind);t.what=what;t.has_animal=animal;t.watered_today=water;t.fed_today=fed;t.cared_today=care;t.fertilizer_available=fert;t.consecutive_dry=dry;t.yield_units=yield;t.pending_care_bonus=bonus;t.planted_day=planted;t.max_lifespan_step=maxlife;t.fertilized_until_day=until;}
        if(!input)return 3;if(c)output<<',';output<<"{\"case\":"<<c<<",\"modes\":[";
        for(int mode=1;mode<=5;++mode){if(mode>1)output<<',';const auto p=compositions::public_crop_forecast::crop_output(o,compositions::public_crop_forecast::Mode(mode));output<<"{\"mode\":"<<mode<<",\"crops\":"<<p.current_crops<<",\"days\":[";
            for(int day=0;day<30;++day){if(day)output<<',';output<<'[';for(int item=0;item<5;++item){if(item)output<<',';output<<p.output[day][item];}output<<']';}output<<"]}";}
        output<<"]}";
    }output<<"]\n";std::cout<<"forecast_cases="<<count<<"\n";
}
