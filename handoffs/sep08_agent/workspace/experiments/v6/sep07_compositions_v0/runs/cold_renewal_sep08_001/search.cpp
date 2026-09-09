#include "../../include/estimate.hpp"
#include "../../include/scenario_io.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <set>

using namespace compositions;
using namespace kag;
namespace initial {
#include "../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/plan.inc"
}
struct Proposal {
    int id=0,herd=0,crop=-1,cap=0,hands=12,added=0;
    std::vector<Life> lives;
    Support support=initial::support;
    EstimatedPlan plan;
    double cash=0,margin=0,minimum=0,score=0;
};
struct Gap {int cell,start,end;};

void service(Life& l) {
    Cohort c{uint8_t(l.item),1,l.start/24,std::min(30,(l.end+23)/24)};
    auto s=productive_service(c,is_crop(l.item)&&CROPS[l.item].ongoing);
    l.water=s.water;l.feed=s.feed;l.care=s.care;l.collect=s.collect_fertilizer;
    l.harvest=s.harvest;l.fertilize=s.fertilize;
}

Proposal make(int herd,int crop,int cap,int hands) {
    Proposal p;p.herd=herd;p.crop=crop;p.cap=cap;p.hands=hands;
    std::vector<Gap> gaps;
    for(auto l:initial::lives) {
        if(l.item==GOOSE) {
            if(herd==3){gaps.push_back({l.y*10+l.x,l.start/24,30});continue;}
            if(herd==1)l.item=COW;
            if(herd==2)l.item=SHEEP;
        }
        p.lives.push_back(l);
    }
    for(int cell=0;cell<100;++cell) {
        std::vector<Life> occupants;
        for(const auto& l:p.lives)if(l.y*10+l.x==cell)occupants.push_back(l);
        std::sort(occupants.begin(),occupants.end(),[](const Life&a,const Life&b){return a.start<b.start;});
        for(size_t j=0;j<occupants.size();++j) {
            const auto& l=occupants[j];if(!is_crop(l.item))continue;
            const int start=(l.end+23)/24,end=j+1<occupants.size()?occupants[j+1].start/24:30;
            if(start+3<=end)gaps.push_back({cell,start,end});
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
        if(!selected.count(g.cell) && int(selected.size())>=cap)continue;
        selected.insert(g.cell);
        for(int day=g.start;day+3<=g.end;) {
            int item=crop==0?WHEAT:crop==1?CARROT:crop==2?((g.cell+day)%2?WHEAT:CARROT):TOMATO;
            if(item==TOMATO && day+12>g.end)item=CARROT;
            const int duration=item==WHEAT?5:item==CARROT?4:12;
            const int end=std::min(g.end,day+duration);
            Life l{item,day*24,std::min(719,end*24),g.cell%10,g.cell/10};
            service(l);p.lives.push_back(l);++p.added;day=end;
        }
    }
    std::stable_sort(p.lives.begin(),p.lives.end(),[](const Life&a,const Life&b){return a.start<b.start;});
    for(auto& l:p.lives)service(l);
    for(int day=13;day<30;++day)p.support.hands[day]=hands;
    return p;
}

void array(std::ostream& out,const auto& a){out<<'[';bool first=true;for(auto v:a){if(!first)out<<',';out<<v;first=false;}out<<']';}
int main(int argc,char**argv) {
    if(argc!=3)return 2;
    auto scenarios=read_scenarios(argv[1]);std::vector<Proposal> proposals;
    for(int herd=0;herd<4;++herd)proposals.push_back(make(herd,-1,0,12));
    for(int herd=0;herd<4;++herd)for(int crop=0;crop<4;++crop)for(int cap:{4,8,16,32})for(int hands:{10,12,14})
        proposals.push_back(make(herd,crop,cap,hands));
    auto start=std::chrono::steady_clock::now();
    EstimateOptions options;options.recorded_support=true;options.service=ServiceModel::Recorded;
    for(int i=0;i<int(proposals.size());++i) {
        auto& p=proposals[i];p.id=i;p.plan=estimate_plan(p.lives,p.support,options);
        if(p.plan.tile_conflicts || p.plan.unplaced_lives)std::abort();
        for(const auto& s:scenarios) {auto e=economics(p.plan.financial,s);p.cash+=e.cash;p.margin+=e.cash-e.rival_cash;p.minimum+=e.min_cash;}
        p.cash/=scenarios.size();p.margin/=scenarios.size();p.minimum/=scenarios.size();
        p.score=p.margin-.5*std::max(0.,-p.minimum)-10*p.plan.uncovered_work;
    }
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::vector<int> order(proposals.size());std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](int a,int b){return proposals[a].score>proposals[b].score;});
    std::set<int> selected{0,1,2,3};
    for(int i=0;i<4;++i)selected.insert(order[i]);
    for(int herd=0;herd<4;++herd)for(int i:order)if(proposals[i].herd==herd&&proposals[i].added){selected.insert(i);break;}
    for(int crop=0;crop<4;++crop)for(int i:order)if(proposals[i].crop==crop){selected.insert(i);break;}
    std::ofstream out(argv[2]);out<<std::setprecision(12)<<"{\"seconds\":"<<seconds<<",\"scenarios\":"<<scenarios.size()<<",\"proposals\":[";
    for(const auto& p:proposals) {
        if(p.id)out<<',';
        out<<"{\"id\":"<<p.id<<",\"herd\":"<<p.herd<<",\"crop\":"<<p.crop<<",\"cap\":"<<p.cap<<",\"hands\":"<<p.hands
           <<",\"added\":"<<p.added<<",\"cash\":"<<p.cash<<",\"margin\":"<<p.margin<<",\"minimum\":"<<p.minimum<<",\"score\":"<<p.score
           <<",\"work_gap\":"<<p.plan.uncovered_work<<",\"selected\":"<<(selected.count(p.id)?"true":"false")<<",\"produced\":";
        array(out,p.plan.produced);out<<",\"daily_hands\":";array(out,p.support.hands);out<<",\"quadrants\":";array(out,p.support.quadrants);out<<",\"lives\":[";
        for(size_t i=0;i<p.lives.size();++i){const auto& l=p.lives[i];if(i)out<<',';array(out,std::array{l.item,l.start,l.end,l.x,l.y,int(l.fertilize),int(l.water),int(l.feed),int(l.care),int(l.collect),int(l.harvest)});}
        out<<"]}";
    }
    out<<"]}\n";
}
