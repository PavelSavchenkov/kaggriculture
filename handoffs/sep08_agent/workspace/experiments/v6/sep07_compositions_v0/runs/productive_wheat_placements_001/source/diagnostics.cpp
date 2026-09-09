#include "evaluation.hpp"
#include "registry.hpp"
int main(int argc,char**argv){
    const auto o=compositions::options(argc,argv);std::ofstream out(o.output);out<<'[';bool comma=false;
    for(uint64_t seed:o.seeds)for(int seat=0;seat<2;++seat){
        auto a=make_agent(o.a),b=make_agent(o.b);const auto result=compositions::run_game(a,b,seed,seat,o);
        if(comma)out<<',';comma=true;out<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash\":"<<result.cash[seat]
            <<",\"opponent_cash\":"<<result.cash[seat^1]<<",\"matched_days\":"<<a.value->matched<<",\"berry_selected\":"<<a.value->berry<<'}';
    }out<<"]\n";
}
