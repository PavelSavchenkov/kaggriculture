#include "season.hpp"
int main(int argc,char** argv){
    if(argc!=3 || fs::exists(argv[2]))return 2;
    const uint64_t seed=std::stoull(argv[1]);const fs::path out=argv[2];
    const Season season(seed);fs::create_directories(out);
    Source own;ContextRival rival;Options options;
    const auto control=run_game(own,rival,seed,0,options);
    for(int p=0;p<2;++p)if(control.hash[p]!=season.hashes[p] || control.cash[p]!=season.final_farms[p].money)std::abort();
    for(const auto& source:season.days){
        const auto g=guarded(source,source.problem,source.own);
        const auto folder=out/"days"/std::to_string(g.plan.day);fs::create_directories(folder);
        save_actions(g.plan.actions,folder/"actions.txt");
        std::ofstream file(folder/"guard.txt");file<<g.plan.day<<' '<<g.quadrants<<'\n';
        for(int c=0;c<100;++c){file<<g.check[c];for(int x:g.tiles[c])file<<' '<<x;file<<'\n';}
        for(int x:g.shed)file<<x<<' ';file<<'\n';for(int x:g.seeds)file<<x<<' ';file<<'\n';
    }
    std::ofstream(out/"CONTROL.json")<<"{\"seed\":"<<seed<<",\"turns\":719,\"full_hash_and_cash_control\":true} \n";
    std::cout<<"exported baseline "<<seed<<'\n';
}
