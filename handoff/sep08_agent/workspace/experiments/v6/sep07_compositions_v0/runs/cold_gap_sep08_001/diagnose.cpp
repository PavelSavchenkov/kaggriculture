#include "../../include/estimate.hpp"
#include "../../include/scenario_io.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace compositions;
using namespace kag;
namespace intended {
#include "../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/plan.inc"
}

void array(std::ostream& out,const auto& values) {
    out<<'[';bool first=true;for(auto v:values){if(!first)out<<',';out<<v;first=false;}out<<']';
}

int main(int argc,char** argv) {
    if(argc!=4)return 2;
    std::ifstream input(argv[1]);std::ofstream out(argv[3]);
    int count;if(!(input>>count))return 2;
    out<<std::setprecision(12)<<"{\"observed_lifetimes\":[";
    for(int k=0;k<count;++k) {
        int n;input>>n;ProductVector regular{},fertilized{},recorded_animals{};
        std::array<int,N_ITEMS> active_days{},births{};
        for(int j=0;j<n;++j) {
            int item,x,y,start,end,born;uint32_t days,water,feed,care,collect,fert;
            input>>item>>x>>y>>start>>end>>born>>days>>water>>feed>>care>>collect>>fert;
            if(!input || item<0 || item>=N_ITEMS)return 2;
            Cohort c{uint8_t(item),1,born,std::min(30,(end+23)/24)};
            ++births[item];active_days[item]+=std::popcount(days);
            for(int mode=0;mode<3;++mode) {
                if(mode==2 && !is_animal(item))continue;
                Service s=productive_service(c,mode==1 || (is_crop(item)&&CROPS[item].ongoing));
                if(mode==2){s.feed=feed;s.care=care;s.collect_fertilizer=collect;}
                auto b=biology(c,s);auto& total=mode==0?regular:mode==1?fertilized:recorded_animals;
                for(const auto& d:b.days)for(int i=0;i<N_PRODUCTS;++i)total[i]+=d.output[i];
            }
        }
        if(k)out<<',';
        out<<"{\"standard_service\":";array(out,regular);
        out<<",\"all_crops_fertilized\":";array(out,fertilized);
        out<<",\"observed_animal_service_daily_harvest\":";array(out,recorded_animals);
        out<<",\"active_days\":";array(out,active_days);out<<",\"births\":";array(out,births);out<<'}';
    }
    if(!input)return 2;
    const auto scenarios=read_scenarios(argv[2]);
    out<<"],\"intended\":[";
    for(int mode=0;mode<2;++mode) {
        EstimateOptions options;options.recorded_support=true;
        options.service=mode?ServiceModel::Fertilized:ServiceModel::Recorded;
        const auto plan=estimate_plan(intended::lives,intended::support,options);
        if(mode)out<<',';
        out<<"{\"mode\":"<<mode<<",\"produced\":";array(out,plan.produced);
        out<<",\"input_wheat\":"<<plan.input_wheat<<",\"input_fertilizer\":"<<plan.input_fertilizer
           <<",\"hire_cost\":"<<plan.hire_cost<<",\"work_gap\":"<<plan.uncovered_work<<",\"scenario_forecasts\":[";
        for(size_t k=0;k<scenarios.size();++k) {
            auto e=economics(plan.financial,scenarios[k]);if(k)out<<',';
            out<<"{\"cash\":"<<e.cash<<",\"rival_cash\":"<<e.rival_cash<<",\"min_cash\":"<<e.min_cash
               <<",\"first_input_gap_step\":"<<e.first_input_gap_step<<",\"excess_orders\":"<<e.excess_order_slots<<'}';
        }
        out<<"]}";
    }
    out<<"]}\n";
}
