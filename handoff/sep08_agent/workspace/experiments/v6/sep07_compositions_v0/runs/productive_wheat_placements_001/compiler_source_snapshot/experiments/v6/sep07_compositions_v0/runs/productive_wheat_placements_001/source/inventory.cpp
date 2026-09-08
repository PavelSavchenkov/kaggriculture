#define main wheat_compiler_main
#include "compile_wheat.cpp"
#undef main

int main(int argc,char**argv){
    if(argc!=2)return 2;
    const fs::path out=argv[1];fs::create_directories(out);
    const Season season(1000);
    std::ofstream lives(out/"crop_lives.csv");
    lives<<"cell,crop,plant,end,water,fertilizer,harvest,output,exact\n";
    for(const auto&l:season.lives){int total=0;for(int q:l.harvested)total+=q;
        lives<<l.cell<<','<<int(l.cohort.item)<<','<<l.cohort.start_day<<','<<l.cohort.end_day<<','<<l.service.water<<','<<l.service.fertilize<<','<<l.service.harvest<<','<<total<<','<<l.exact<<'\n';}
    std::ofstream inputs(out/"source_daily.csv");
    inputs<<"day,fertilizer_shed,fertilizer_carried,fertilizer_bought,fertilizer_sold,fertilizer_discarded,hires,hired_cost,end_shed\n";
    for(int day=0;day<29;++day){const auto&a=season.days[day].start.st.farms[0];const auto&b=season.days[day+1].start.st.farms[0];
        int carried=0,occupied=0;for(int u=0;u<a.n_units;++u)carried+=a.inv[u][FERTILIZER];for(int q:b.shed)occupied+=q;
        int bought=0,hires=0,cost=0;
        for(const auto&action:season.days[day].own)for(int n=0;n<action.n_orders;++n){const auto&o=action.orders[n];
            if(o.op==M_BUY_PRODUCT&&o.item==FERTILIZER)bought+=o.n;
            if(o.op==M_HIRE)for(int i=0;i<o.n;++i)cost+=fib(hires++);}
        inputs<<day<<','<<a.shed[FERTILIZER]<<','<<carried<<','<<bought<<','<<b.sold_units[FERTILIZER]-a.sold_units[FERTILIZER]<<','<<b.discarded[FERTILIZER]-a.discarded[FERTILIZER]<<','<<hires<<','<<cost<<','<<occupied<<'\n';}
    std::ofstream visits(out/"visits.csv");visits<<"cell,day,source_jobs,new_jobs,source_flexible_visits,added_jobs,visit_deficit,source_hires\n";
    std::ofstream eligible(out/"eligible.csv");eligible<<"cell,first,cycles\n";
    for(int cell=0;cell<100;++cell){
        const auto work=source_work(season.days[13].problem,cell);
        const bool plants=std::any_of(work.begin(),work.end(),[](auto&a){return a.op==OP_PLANT&&a.arg==WHEAT;});
        if(!plants)continue;
        Rotation r;r.cell=cell;
        bool crop_only=true;for(int day=13;day<29;++day)for(const auto&a:source_work(season.days[day].problem,cell))
            if(a.op==OP_BUILD_COOP||a.op==OP_BUILD_PASTURE||a.op==OP_PLACE)crop_only=false;
        if(!crop_only)continue;
        eligible<<cell<<",13,3\n";
        estimate_rotation(season,r,out/("estimate_"+std::to_string(cell)+".json"));
        for(int day=13;day<29;++day){const auto&d=season.days[day];auto sim=d.start;int count=0;
            for(int h=0;h<24;++h){const auto&farm=sim.st.farms[0];for(int u=0;u<farm.n_units;++u){const int op=d.own[h].units[u].op;
                if(farm.pos_y[u]*10+farm.pos_x[u]==cell&&(op==OP_PASS||(op>=OP_PLANT&&op<=OP_CARE)))++count;}
                sim.step(d.own[h],d.rival[h]);}
            auto original=source_work(d.problem,cell);const auto replacement=rotation_work(r,day,{},original);
            visits<<cell<<','<<day<<','<<original.size()<<','<<replacement.size()<<','<<count<<','<<int(replacement.size())-int(original.size())<<','<<std::max(0,int(replacement.size())-count)<<','<<d.problem.worker_count-1<<'\n';
        }
    }
}
