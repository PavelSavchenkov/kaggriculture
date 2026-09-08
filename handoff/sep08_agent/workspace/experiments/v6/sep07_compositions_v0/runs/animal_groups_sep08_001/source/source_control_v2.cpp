#include "season.hpp"

int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;
    const fs::path out(argv[1]);fs::create_directories(out);
    std::ofstream report(out/"CONTROLS.csv");report<<"seed,day,requirements,invariants,errors\n";
    for(uint64_t seed:{1000,1001}){
        const Season season(seed);
        for(int day=8;day<29;++day){
            auto p=season.days[day].problem;day_scheduler::prepare_problem(p);
            auto actions=season.days[day].own;
            for(int h=0;h<24;++h){
                auto& a=actions[h];
                std::array<int,N_ITEMS> net{};
                for(int i=0;i<N_ITEMS;++i)net[i]=p.shed_availability[h][i]-(h?p.shed_availability[h-1][i]:0);
                for(int j=0;j<a.n_orders;++j){
                    auto& order=a.orders[j];
                    if(order.op==M_SELL){order.n=std::min(order.n,net[order.item]);net[order.item]-=order.n;if(!order.n)order={};}
                    else order={};
                }
                for(const auto& event:p.market_plan)if(event.hour==h){
                    a.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
                    a.n_orders=std::max(a.n_orders,int(event.order_index)+1);
                }
                for(int n:net)if(n)std::abort();
                for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL)a.orders[j]={};
                a.finalize();
            }
            const auto replay=replay_schedule(p,actions);
            if(!replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())std::abort();
            report<<seed<<','<<day<<','<<replay.requirements_satisfied<<','<<replay.invariants_satisfied<<','<<replay.errors.size()<<'\n';
            const auto folder=out/(std::to_string(seed)+"_"+std::to_string(day));fs::create_directories(folder);
            save_problem_json(p,folder/"problem.json");save_actions(actions,folder/"physical_actions.txt");
            std::ofstream errors(folder/"errors.txt");for(const auto& error:replay.errors)errors<<error<<'\n';
        }
    }
}
