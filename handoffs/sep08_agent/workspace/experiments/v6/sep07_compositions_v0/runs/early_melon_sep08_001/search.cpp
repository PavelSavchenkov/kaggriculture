#include "../../include/estimate.hpp"
#include "../../include/scenario_io.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <set>

using namespace compositions;
using namespace kag;
struct Plan {int base=0,mode=0,herd=0,crop=-1,cap=0,hands=12;std::vector<Life> lives;Support support;};
struct Gap {int cell,start,end;};
void service(Life& l) {
    Cohort c{uint8_t(l.item),1,l.start/24,std::min(30,(l.end+23)/24)};
    auto s=productive_service(c,is_crop(l.item)&&CROPS[l.item].ongoing);
    l.water=s.water;l.feed=s.feed;l.care=s.care;l.collect=s.collect_fertilizer;l.harvest=s.harvest;l.fertilize=s.fertilize;
}
Plan modify(const Plan& original,const Plan& base,int mode) {
    Plan p=original;p.mode=mode;
    if(!mode)return p;
    if(mode==1) {
        for(auto& l:p.lives)if(l.item==MELON){l.end=std::min(l.end,(l.start/24+11)*24);service(l);}
        return p;
    }
    p.lives.clear();std::vector<Gap> gaps;
    for(auto l:base.lives) {
        if(l.item==GOOSE) {
            if(p.herd==3){gaps.push_back({l.y*10+l.x,l.start,719});continue;}
            if(p.herd==1)l.item=COW;
            if(p.herd==2)l.item=SHEEP;
        }
        if(l.item==MELON)l.end=std::min(l.end,(l.start/24+10)*24+(mode==3?12:24));
        service(l);p.lives.push_back(l);
    }
    for(int cell=0;cell<100;++cell) {
        std::vector<Life> occupants;
        for(const auto& l:p.lives)if(l.y*10+l.x==cell)occupants.push_back(l);
        std::sort(occupants.begin(),occupants.end(),[](const Life&a,const Life&b){return a.start<b.start;});
        for(size_t j=0;j<occupants.size();++j) {
            const auto& l=occupants[j];if(!is_crop(l.item))continue;
            const int start=mode==3?l.end:((l.end+23)/24)*24;
            const int end=j+1<occupants.size()?occupants[j+1].start:719;
            if(start/24+2<=(end-1)/24)gaps.push_back({cell,start,end});
        }
    }
    std::sort(gaps.begin(),gaps.end(),[](const Gap&a,const Gap&b){
        const double va=(a.end-a.start)/double(2+shed_distance(a.cell%10,a.cell/10));
        const double vb=(b.end-b.start)/double(2+shed_distance(b.cell%10,b.cell/10));
        if(va!=vb)return va>vb;
        if(a.start!=b.start)return a.start<b.start;
        return a.cell<b.cell;
    });
    std::set<int> selected;
    for(const auto& g:gaps) {
        if(!selected.count(g.cell)&&int(selected.size())>=p.cap)continue;
        selected.insert(g.cell);
        for(int start=g.start;start/24+2<=(g.end-1)/24;) {
            const int day=start/24;
            int item=p.crop==0?WHEAT:p.crop==1?CARROT:p.crop==2?((g.cell+day)%2?WHEAT:CARROT):TOMATO;
            if(item==TOMATO&&day+11>(g.end-1)/24)item=CARROT;
            const int duration=item==WHEAT?5:item==CARROT?4:12;
            const int end=std::min(g.end,(day+duration)*24);
            Life l{item,start,end,g.cell%10,g.cell/10};service(l);p.lives.push_back(l);start=end;
        }
    }
    std::stable_sort(p.lives.begin(),p.lives.end(),[](const Life&a,const Life&b){return a.start<b.start;});
    return p;
}
void array(std::ostream& out,const auto& a){out<<'[';bool first=true;for(auto v:a){if(!first)out<<',';out<<v;first=false;}out<<']';}
int main(int argc,char**argv) {
    if(argc!=4)return 2;
    std::ifstream input(argv[1]);int count;input>>count;if(!input||count!=4)return 2;
    std::vector<Plan> sources(count),plans;
    for(auto& p:sources) {
        int n;input>>p.base>>p.herd>>p.crop>>p.cap>>p.hands>>n;
        for(auto& v:p.support.hands)input>>v;for(auto& v:p.support.quadrants)input>>v;
        p.lives.resize(n);
        for(auto& l:p.lives)input>>l.item>>l.start>>l.end>>l.x>>l.y>>l.fertilize>>l.water>>l.feed>>l.care>>l.collect>>l.harvest;
    }
    if(!input)return 2;
    for(const auto& p:sources)for(int mode=0;mode<(p.base?4:2);++mode)plans.push_back(modify(p,sources[0],mode));
    auto scenarios=read_scenarios(argv[2]);
    EstimateOptions options;options.recorded_support=true;options.service=ServiceModel::Recorded;
    std::ofstream out(argv[3]);out<<std::setprecision(12)<<"{\"plans\":[";
    auto start=std::chrono::steady_clock::now();
    bool first=true;
    for(const auto& p:plans) {
        auto e=estimate_plan(p.lives,p.support,options);if(e.tile_conflicts||e.unplaced_lives)return 3;
        double cash=0,margin=0,minimum=0;
        for(const auto& s:scenarios){auto f=economics(e.financial,s);cash+=f.cash;margin+=f.cash-f.rival_cash;minimum+=f.min_cash;}
        cash/=scenarios.size();margin/=scenarios.size();minimum/=scenarios.size();
        if(!first)out<<',';first=false;
        out<<"{\"base\":"<<p.base<<",\"mode\":"<<p.mode<<",\"cash\":"<<cash<<",\"margin\":"<<margin<<",\"minimum\":"<<minimum
           <<",\"work_gap\":"<<e.uncovered_work<<",\"produced\":";array(out,e.produced);
        out<<",\"daily_hands\":";array(out,p.support.hands);out<<",\"quadrants\":";array(out,p.support.quadrants);out<<",\"lives\":[";
        for(size_t i=0;i<p.lives.size();++i){const auto& l=p.lives[i];if(i)out<<',';array(out,std::array{l.item,l.start,l.end,l.x,l.y,int(l.fertilize),int(l.water),int(l.feed),int(l.care),int(l.collect),int(l.harvest)});}
        out<<"]}";
    }
    out<<"],\"scenarios\":"<<scenarios.size()<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"}\n";
}
