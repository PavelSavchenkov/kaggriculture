#include "../../include/estimate.hpp"
#include "../../include/scenario_io.hpp"
#include "../compiler_labor_sep08_001/cold_farm.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>

using namespace compositions;
using namespace kag;

struct Proposal {
    int id=0,day=0,count=0,hands=8;
    std::string family;
    std::vector<Life> lives;
    Support support;
    ProductVector output{};
    double cash=0,margin=0,minimum=0,score=0;
    int work_gap=0;
};

// Same owned-first placement rule as compiler_placement_sep08_001. Placement
// is materialized once, then passed unchanged to economics and execution.
void place(Proposal& p) {
    std::stable_sort(p.lives.begin(),p.lives.end(),[](const Life& a,const Life& b){return a.start<b.start;});
    std::array<int,100> release{},previous{};previous.fill(-1);
    int land=1;
    for(auto& life:p.lives) {
        bool owned=false;
        for(int cell=0;cell<100;++cell)
            owned|=release[cell]<=life.start && quadrant_of(cell%10,cell/10,10)<land;
        int best=-1;double score=1e100;
        for(int cell=0;cell<100;++cell)if(release[cell]<=life.start) {
            int x=cell%10,y=cell/10,q=quadrant_of(x,y,10);
            if(owned && q>=land)continue;
            double v=5*q+shed_distance(x,y)*(is_animal(life.item)?2.0:1.0)
                -(is_animal(life.item)&&is_animal(previous[cell])?1.5:0.0)+cell*.0001;
            if(v<score){score=v;best=cell;}
        }
        if(best<0)std::abort();
        life.x=best%10;life.y=best/10;
        release[best]=life.end;previous[best]=life.item;
        land=std::max(land,quadrant_of(life.x,life.y,10)+1);
        for(int d=life.start/24;d<30;++d)p.support.quadrants[d]=std::max(p.support.quadrants[d],land);
        Cohort cohort{uint8_t(life.item),1,life.start/24,std::min(30,(life.end+23)/24)};
        auto service=productive_service(cohort,is_crop(life.item)&&CROPS[life.item].ongoing);
        life.water=service.water;life.feed=service.feed;life.care=service.care;
        life.collect=service.collect_fertilizer;life.harvest=service.harvest;life.fertilize=service.fertilize;
    }
    for(int d=0;d<30;++d)p.support.hands[d]=d<p.day?8:p.hands;
}

void add(Proposal& p,int item,int count,int day,int end) {
    for(int i=0;i<count;++i)p.lives.push_back({item,24*day,std::min(719,24*end),0,0});
}

Proposal make(std::string family,int count,int day,int hands) {
    Proposal p;p.family=family;p.count=count;p.day=day;p.hands=hands;
    p.lives=compiler_labor_data::cold_farm(2,2,0,7,12,8);
    if(family=="goose")add(p,GOOSE,count,day,30);
    if(family=="cow")add(p,COW,count,day,30);
    if(family=="sheep")add(p,SHEEP,count,day,30);
    if(family=="mixed" || family=="mixed_berries") {
        add(p,COW,count/3,day,30);add(p,SHEEP,count/3,day,30);add(p,GOOSE,count-2*(count/3),day,30);
    }
    if(family=="strawberry" || family=="mixed_berries")add(p,STRAWBERRY,family=="mixed_berries"?16:count,day,std::min(30,day+17));
    if(family=="tomato")add(p,TOMATO,count,day,std::min(30,day+12));
    if(family=="melon")add(p,MELON,count,day,std::min(30,day+13));
    place(p);return p;
}

void array(std::ostream& out,const auto& values) {
    out<<'[';bool first=true;for(auto v:values){if(!first)out<<',';first=false;out<<v;}out<<']';
}

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    auto scenarios=read_scenarios(argv[1]);
    std::filesystem::path out=argv[2];
    if(std::filesystem::exists(out))return 2;
    std::filesystem::create_directories(out);
    std::vector<Proposal> proposals;
    for(int hands:{8,10,12})proposals.push_back(make("control",0,6,hands));
    for(const std::string family:{"goose","cow","sheep","mixed"})
        for(int day:{4,6,8,10,12})for(int count:{3,6,9,12})for(int hands:{8,10,12})
            proposals.push_back(make(family,count,day,hands));
    for(const std::string family:{"strawberry","tomato","melon"})
        for(int day:{4,6,8,10})for(int count:{8,16,24})for(int hands:{8,10,12})
            proposals.push_back(make(family,count,day,hands));
    for(int day:{6,8,10,12})for(int count:{6,12})for(int hands:{8,10,12})
        proposals.push_back(make("mixed_berries",count,day,hands));
    const auto start=std::chrono::steady_clock::now();
    EstimateOptions options;options.recorded_layout=true;options.recorded_support=true;options.service=ServiceModel::Recorded;
    for(int i=0;i<int(proposals.size());++i) {
        auto& p=proposals[i];p.id=i;
        auto plan=estimate_plan(p.lives,p.support,options);
        if(plan.tile_conflicts || plan.unplaced_lives)std::abort();
        p.output=plan.produced;p.work_gap=plan.uncovered_work;
        for(const auto& scenario:scenarios) {
            auto e=economics(plan.financial,scenario);
            p.cash+=e.cash;p.margin+=e.cash-e.rival_cash;p.minimum+=e.min_cash;
        }
        p.cash/=scenarios.size();p.margin/=scenarios.size();p.minimum/=scenarios.size();
        // Both warnings remain in the report. These are approximate penalties,
        // not cash credit, feasibility certificates or exact rival responses.
        p.score=p.margin-.5*std::max(0.,-p.minimum)-10*p.work_gap;
    }
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::vector<int> order(proposals.size());std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](int a,int b){return proposals[a].score>proposals[b].score;});
    std::set<int> selected{0,1,2};
    for(int i=0;i<12;++i)selected.insert(order[i]);
    // Retain whole-family alternatives and all workforce levels even when
    // imperfect economics/route estimates rank one family poorly.
    for(const std::string family:{"goose","cow","sheep","mixed","strawberry","tomato","melon","mixed_berries"})
        for(int hands:{8,10,12})for(int i:order)
            if(proposals[i].family==family && proposals[i].hands==hands){selected.insert(i);break;}
    std::ofstream json(out/"proposals.json");json<<std::setprecision(12)<<'[';
    for(size_t i=0;i<proposals.size();++i) {
        const auto& p=proposals[i];if(i)json<<',';
        json<<"{\"id\":"<<p.id<<",\"family\":\""<<p.family<<"\",\"day\":"<<p.day<<",\"count\":"<<p.count
            <<",\"hands\":"<<p.hands<<",\"estimated_cash\":"<<p.cash<<",\"estimated_margin\":"<<p.margin
            <<",\"min_cash\":"<<p.minimum<<",\"work_gap\":"<<p.work_gap<<",\"score\":"<<p.score
            <<",\"selected\":"<<(selected.count(i)?"true":"false")<<",\"produced\":";array(json,p.output);
        json<<",\"daily_hands\":";array(json,p.support.hands);json<<",\"quadrants\":";array(json,p.support.quadrants);
        json<<",\"lives\":[";
        for(size_t j=0;j<p.lives.size();++j){const auto& l=p.lives[j];if(j)json<<',';json<<'['<<l.item<<','<<l.start<<','<<l.end<<','<<l.x<<','<<l.y<<','<<l.fertilize<<','<<l.water<<','<<l.feed<<','<<l.care<<','<<l.collect<<','<<l.harvest<<']';}
        json<<"]}";
    }
    json<<"]\n";
    std::ofstream summary(out/"SUMMARY.json");
    summary<<"{\"estimated\":"<<proposals.size()<<",\"scenarios\":"<<scenarios.size()<<",\"seconds\":"<<seconds
        <<",\"selected\":"<<selected.size()<<",\"best_estimated_id\":"<<order[0]<<"}\n";
    std::printf("Estimated %zu proposals x %zu scenarios in %.3fs; selected %zu.\n",proposals.size(),scenarios.size(),seconds,selected.size());
}
