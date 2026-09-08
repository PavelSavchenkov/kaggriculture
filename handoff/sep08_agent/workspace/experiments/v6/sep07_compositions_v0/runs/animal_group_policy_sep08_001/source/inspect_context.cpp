#include "season.hpp"
std::string signature(const DayProblem& p,int cell){
    std::ostringstream s;for(const auto& w:p.tile_work)if(w.tile==cell)
        for(const auto& a:w.actions)s<<+a.op<<':'<<a.arg<<':'<<a.quantity<<':'<<a.output_item<<':'<<a.output_quantity<<';';
    return s.str();
}
int main(){
    Season a(1014),b(1019);
    std::cout<<"Source public-router controls: "<<a.final_farms[0].money<<' '<<b.final_farms[0].money<<'\n';
    for(int day=15;day<30;++day){
        const auto& x=a.days[day].problem;const auto& y=b.days[day].problem;
        for(int c=0;c<100;++c)if(signature(x,c)!=signature(y,c))
            std::cout<<"day="<<day<<" cell="<<c<<" a="<<signature(x,c)<<" b="<<signature(y,c)<<'\n';
        if(day==23){
            std::cout<<"day23 workers "<<x.worker_count<<' '<<y.worker_count<<" start_shed";
            for(int i=0;i<N_ITEMS;++i)std::cout<<' '<<x.start.shed[i]<<':'<<y.start.shed[i];std::cout<<'\n';
        }
    }
}
