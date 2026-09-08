// A constructive final-harvest repair, checked by the independent physical replay.
#include "day_solver/io.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace day_solver;
using namespace kag;
namespace fs=std::filesystem;

std::array<Action,24> read(const fs::path& path){
    std::ifstream in(path);if(!in)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        in>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;in>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;in>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!in)std::abort();return result;
}
void save(const std::array<Action,24>& actions,const fs::path& path){
    std::ofstream out(path);
    for(const auto& a:actions){
        out<<a.n_units<<' '<<a.n_orders;
        for(int u=0;u<a.n_units;++u)out<<' '<<+a.units[u].op<<' '<<+a.units[u].arg<<' '<<a.units[u].n;
        for(int s=0;s<a.n_orders;++s)out<<' '<<+a.orders[s].op<<' '<<+a.orders[s].item<<' '<<a.orders[s].n;
        out<<'\n';
    }
}
int main(int argc,char** argv){
    if(argc<5 || fs::exists(argv[3]))return 2;
    const auto problem=load_problem_json(argv[1]);const auto original=read(argv[2]);
    const fs::path out=argv[3];fs::create_directories(out);
    std::vector<int> cells;for(int i=4;i<argc;++i)cells.push_back(std::stoi(argv[i]));
    std::sort(cells.begin(),cells.end());
    const int worker=original[22].n_units;
    if(problem.worker_count<=worker)return 2;
    auto base=original;std::vector<int> releases{0};auto events=problem.market_plan;
    std::sort(events.begin(),events.end(),[](auto a,auto b){return std::pair(a.hour,a.order_index)<std::pair(b.hour,b.order_index);});
    for(const auto& e:events)if(e.market_op==M_HIRE)releases.push_back(e.hour+1);
    for(int h=0;h<24;++h){
        base[h].n_units=std::count_if(releases.begin(),releases.end(),[&](int r){return r<=h;});
        for(int u=original[h].n_units;u<base[h].n_units;++u)base[h].units[u]={};
        base[h].n_orders=0;for(auto& order:base[h].orders)order={};
        for(const auto& e:events)if(e.hour==h){
            base[h].orders[e.order_index]={e.market_op,uint8_t(std::max(0,int(e.item))),e.quantity};
            base[h].n_orders=std::max(base[h].n_orders,int(e.order_index)+1);
        }
        base[h].finalize();
    }
    int attempts=0,passed=0;
    do for(int start:{44,45,54,55})for(int home:{44,45,54,55}){
        auto actions=base;int hour=releases[worker],at=start;
        auto emit=[&](int op){if(hour>=23)return false;actions[hour++].units[worker]={uint8_t(op),0,1};return true;};
        auto walk=[&](int cell){
            while(at%10!=cell%10){const int direction=cell%10>at%10?1:-1;if(!emit(direction>0?OP_EAST:OP_WEST))return false;at+=direction;}
            while(at/10!=cell/10){const int direction=cell/10>at/10?10:-10;if(!emit(direction>0?OP_SOUTH:OP_NORTH))return false;at+=direction;}
            return true;
        };
        bool fits=true;
        for(int cell:cells){if(!walk(cell) || !emit(OP_HARVEST)){fits=false;break;}}
        if(!fits || !walk(home) || !emit(OP_DROP))continue;
        for(auto& a:actions)a.finalize();
        const auto replay=replay_schedule(problem,actions);++attempts;
        const bool valid=replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty();
        std::ofstream errors(out/("attempt_"+std::to_string(attempts)+".txt"));
        errors<<"start="<<start<<" home="<<home<<" requirements="<<replay.requirements_satisfied<<" invariants="<<replay.invariants_satisfied<<'\n';
        for(const auto& error:replay.errors)errors<<error<<'\n';
        if(valid){++passed;save(actions,out/"actions.txt");break;}
    }while(!passed && std::next_permutation(cells.begin(),cells.end()));
    std::ofstream(out/"STATUS.json")<<"{\"attempts\":"<<attempts<<",\"passed\":"<<passed<<"}\n";
    std::cout<<"attempts="<<attempts<<" passed="<<passed<<std::endl;
}
