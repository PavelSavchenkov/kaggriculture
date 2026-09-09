#define main reuse_tool_main
#include "reuse.cpp"
#undef main

int main(int argc,char** argv) {
    if(argc!=5 || fs::exists(argv[4]))return 2;
    const auto source=load_problem_json(argv[1]);
    const auto markets=read_actions(argv[2]);
    const auto route=read_actions(argv[3]);
    const fs::path out(argv[4]);fs::create_directories(out);
    std::ofstream log(out/"trials.csv");log<<"sale_hour,requirements,invariants\n";
    for(int hour=18;hour<24;++hour) {
        auto problem=source;
        for(int h=17;h<hour;++h){if(problem.shed_availability[h][STRAWBERRY]<1)return 2;--problem.shed_availability[h][STRAWBERRY];}
        day_scheduler::prepare_problem(problem);
        auto actions=route;
        for(auto& action:actions){action.n_orders=0;for(auto& order:action.orders)order={};}
        for(const auto& event:problem.market_plan) {
            auto& action=actions[event.hour];
            action.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
            action.n_orders=std::max<int>(action.n_orders,event.order_index+1);
        }
        for(auto& action:actions)action.finalize();
        auto replay=replay_schedule(problem,actions);
        int drop_hour=-1,drop_unit=-1;
        for(int h=0;h<=hour && !replay.requirements_satisfied;++h)for(int u=0;u<actions[h].n_units && !replay.requirements_satisfied;++u) {
            if(actions[h].units[u].op!=OP_PASS)continue;
            auto trial=actions;trial[h].units[u]={OP_DROP,0,1};trial[h].finalize();
            const auto check=replay_schedule(problem,trial);
            if(check.requirements_satisfied && check.invariants_satisfied){actions=trial;replay=check;drop_hour=h;drop_unit=u;}
        }
        log<<hour<<','<<replay.requirements_satisfied<<','<<replay.invariants_satisfied<<'\n';log.flush();
        for(const auto& error:replay.errors)std::cerr<<"hour "<<hour<<": "<<error<<'\n';
        if(!replay.requirements_satisfied || !replay.invariants_satisfied)continue;
        for(int h=0;h<24;++h) {
            actions[h].n_orders=markets[h].n_orders;
            std::copy_n(markets[h].orders,markets[h].n_orders,actions[h].orders);
            for(int s=0;s<actions[h].n_orders;++s)if(actions[h].orders[s].op==M_HIRE)actions[h].orders[s]={};
        }
        for(const auto& event:problem.market_plan)if(event.market_op==M_HIRE) {
            auto& a=actions[event.hour];a.orders[event.order_index]={M_HIRE,0,event.quantity};
            a.n_orders=std::max<int>(a.n_orders,event.order_index+1);
        }
        bool removed=false;
        for(int s=0;s<actions[17].n_orders && !removed;++s) {
            auto& order=actions[17].orders[s];
            if(order.op==M_SELL && order.item==STRAWBERRY && order.n>0){--order.n;removed=true;if(!order.n)order={};}
        }
        if(!removed)return 3;
        bool placed=false;
        for(int s=0;s<actions[hour].n_orders && !placed;++s) {
            auto& order=actions[hour].orders[s];
            if(order.op==M_SELL && order.item==STRAWBERRY){++order.n;placed=true;}
        }
        if(!placed && actions[hour].n_orders<10){actions[hour].orders[actions[hour].n_orders++]={M_SELL,STRAWBERRY,1};placed=true;}
        if(!placed)return 3;
        for(auto& action:actions)action.finalize();
        const auto folder=out/("hour"+std::to_string(hour));fs::create_directories(folder);
        save_problem_json(problem,folder/"problem.json");write_actions(folder/"actions.txt",actions);
        std::ofstream(folder/"STATUS.json")<<"{\"physical_schedule\":true,\"worker_count\":"<<problem.worker_count
            <<",\"moved_strawberry_units\":1,\"from_hour\":17,\"to_hour\":"<<hour<<",\"drop_hour\":"<<drop_hour<<",\"drop_unit\":"<<drop_unit<<",\"full_game_check_required\":true}\n";
        std::cout<<"valid physical route, strawberry sale hour "<<hour<<'\n';
    }
}
