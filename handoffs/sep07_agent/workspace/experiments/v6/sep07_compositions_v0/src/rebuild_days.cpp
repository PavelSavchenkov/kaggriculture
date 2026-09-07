#include "../include/day_contract.hpp"
using namespace compositions::day_contract;

int main(int argc,char** argv) {
    if(argc!=4) {std::cerr<<"usage: rebuild_days PROGRAM OUTPUT_DIRECTORY SECONDS_PER_DAY\n";return 2;}
    const int program=std::stoi(argv[1]);const double seconds=std::stod(argv[3]);
    const std::filesystem::path directory=argv[2];std::filesystem::create_directories(directory);
    top_replay_library::Agent agent(program);public_router::Agent rival;
    Config config;config.seed=1000;Sim sim(config);
    agent.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=1000^0xa37108e62d045fb9ULL;
    std::array<uint8_t,8> shops;for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::ofstream report(directory/"days.csv");
    report<<"day,workers,tile_operations,discarded,source_contract_valid,solved,seconds,full_game_equal,note\n";
    for(int day=0;day<29;++day) {
        RecordedDay recorded(sim);
        for(int hour=0;hour<24;++hour) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            Action actions[2];
            agent.act(agent::runtime::make_observation(sim,0),{},actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),{},actions[1]);
            const auto before=sim;sim.step(actions[0],actions[1]);
            append_contract(recorded,before,sim,actions);
        }
        int operations=0;for(const auto& work:recorded.problem.tile_work)operations+=work.actions.size();
        report<<day<<','<<recorded.problem.worker_count<<','<<operations<<','<<recorded.discarded<<',';
        if(recorded.discarded) {report<<"0,0,0,0,source_overflow_outside_solver_contract\n";continue;}
        try {
            day_scheduler::prepare_problem(recorded.problem);
            const auto prefix=directory/("day_"+std::to_string(day));
            save_problem_json(recorded.problem,prefix.string()+"_problem.json");
            save_actions(recorded.own,prefix.string()+"_source.txt");
            auto source=recorded.own;
            for(auto& action:source) {action.n_orders=0;std::fill(std::begin(action.orders),std::end(action.orders),Order{});action.finalize();}
            for(const auto& event:recorded.problem.market_plan) {
                auto& action=source[event.hour];action.n_orders=std::max(action.n_orders,int(event.order_index)+1);
                action.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
            }
            for(auto& action:source)action.finalize();
            const auto reference=replay_schedule(recorded.problem,source);
            if(!reference.requirements_satisfied || !reference.invariants_satisfied) {
                report<<"0,0,0,0,source_contract_replay_failed\n";
                std::ofstream errors(directory/("day_"+std::to_string(day)+"_errors.txt"));
                for(const auto& error:reference.errors)errors<<error<<'\n';
                continue;
            }
            day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
            auto result=day_scheduler::solve(recorded.problem,options);
            bool equal=false;
            if(result.schedule) {
                save_actions(*result.schedule,prefix.string()+"_schedule.txt");
                auto rebuilt=recorded.start;
                for(int hour=0;hour<24;++hour) {
                    std::copy_n(shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                    auto action=(*result.schedule)[hour];
                    // Restore the exact original market course. This validation
                    // is conditional on the recorded rival course, not adaptive
                    // full-game deployment or a cash-insensitive solver claim.
                    action.n_orders=recorded.own[hour].n_orders;
                    std::copy_n(recorded.own[hour].orders,action.n_orders,action.orders);action.finalize();
                    rebuilt.step(action,recorded.rival[hour]);
                }
                const auto& a=sim.st.farms[0];const auto& b=rebuilt.st.farms[0];
                equal=a.money==b.money && a.n_quadrants==b.n_quadrants;
                for(int item=0;item<N_ITEMS;++item)equal&=a.shed[item]==b.shed[item] && a.produced[item]==b.produced[item];
                for(int item=0;item<N_CROPS;++item)equal&=a.seeds[item]==b.seeds[item];
                for(int y=0;y<10;++y)for(int x=0;x<10;++x)equal&=managed(a.tiles[y][x],day+1)==managed(b.tiles[y][x],day+1);
                if(!equal) {
                    std::ofstream difference(prefix.string()+"_full_difference.txt");
                    difference<<"cash "<<a.money<<' '<<b.money<<" land "<<a.n_quadrants<<' '<<b.n_quadrants<<'\n';
                    for(int item=0;item<N_ITEMS;++item)
                        if(a.shed[item]!=b.shed[item] || a.produced[item]!=b.produced[item] || a.discarded[item]!=b.discarded[item])
                            difference<<"item "<<item<<" shed "<<a.shed[item]<<' '<<b.shed[item]<<" produced "<<a.produced[item]<<' '<<b.produced[item]
                                <<" discarded "<<a.discarded[item]<<' '<<b.discarded[item]<<'\n';
                    for(int y=0;y<10;++y)for(int x=0;x<10;++x)
                        if(managed(a.tiles[y][x],day+1)!=managed(b.tiles[y][x],day+1))difference<<"tile "<<x<<' '<<y<<'\n';
                }
            }
            report<<"1,"<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<",conditional_rebuild\n";
            std::cout<<"day="<<day<<" solved="<<bool(result.schedule)<<" seconds="<<result.seconds<<" full_equal="<<equal<<std::endl;
        } catch(const std::exception& error) {
            report<<"0,0,0,0,invalid_contract\n";
            std::ofstream errors(directory/("day_"+std::to_string(day)+"_errors.txt"));errors<<error.what()<<'\n';
        }
        report.flush();
    }
}
