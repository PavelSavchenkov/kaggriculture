#include "evaluation.hpp"
#include "registry.hpp"
#include <iomanip>

int main(int argc,char**argv){
    const auto o=compositions::options(argc,argv);std::ofstream out(o.output);out<<std::setprecision(17)<<"[";bool comma=false;
    for(uint64_t seed:o.seeds)for(int seat=0;seat<2;++seat){
        auto a=make_agent(o.a),b=make_agent(o.b);const auto result=compositions::run_game(a,b,seed,seat,o);const auto& t=a.value->trace;
        if(comma)out<<",";comma=true;
        out<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"branch\":"<<t.branch<<",\"cash_before288\":"<<t.cash<<",\"rival_cash_before288\":"<<t.rival_cash<<",\"shed_wheat_before288\":"<<t.wheat;
        out<<",\"cash\":"<<result.cash[seat]<<",\"opponent_cash\":"<<result.cash[seat^1]<<",\"estimates\":[";
        for(int i=0;i<2;++i){if(i)out<<",";out<<"{\"own\":"<<t.estimates[i].own<<",\"rival\":"<<t.estimates[i].rival<<",\"min_cash\":"<<t.estimates[i].min_cash<<"}";}
        out<<"],\"physical_before288\":[";for(size_t i=0;i<t.physical.size();++i){if(i)out<<",";out<<t.physical[i];}out<<"]}";
    }
    out<<"]\n";
}
