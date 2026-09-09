#include "../../include/day_contract.hpp"
#include "../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
#include <iomanip>

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

bool production(int item,int born,int day) {
    const auto& rule=ANIMALS[item-GOOSE];const int age=day-born-rule.first_yield_day;
    return age>=0 && age%rule.interval==0;
}

int add_care(const RecordedDay& source,DayProblem& problem) {
    const int day=source.start.st.day;
    int added=0;
    for(auto& work:problem.tile_work) {
        const auto& tile=source.start.st.farms[0].tiles[work.tile/10][work.tile%10];
        if(!tile.has_animal)continue;
        bool fed=false,cared=false,future=false;
        for(const auto& action:work.actions){fed|=action.op==OP_FEED;cared|=action.op==OP_CARE;}
        for(int d=day+2;d<30;++d)future|=production(tile.what,tile.planted_day,d);
        const auto& rule=ANIMALS[tile.what-GOOSE];
        if(!fed || cared || !future || (tile.pending_care_bonus>=rule.max_held-1 && !production(tile.what,tile.planted_day,day+1)))continue;
        auto end=std::find_if(problem.required_end_tiles.begin(),problem.required_end_tiles.end(),[&](const auto& e){return e.tile==work.tile;});
        if(end==problem.required_end_tiles.end() || !end->exact_state || end->exact_state->animal!=tile.what)std::abort();
        TileWorkAction action;action.op=OP_CARE;work.actions.push_back(action);
        ++end->exact_state->pending_care_bonus;++added;
    }
    return added;
}

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    const fs::path out=argv[1];double seconds=std::stod(argv[2]);
    if(fs::exists(out) || seconds<=0)return 2;fs::create_directories(out);
    Config config;config.seed=1000;Sim sim(config);
    dated_expansion_p362::Agent own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=1000^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
    for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::vector<RecordedDay> days;days.reserve(30);
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    for(int step=0;step<719;++step) {
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        if(sim.st.hour==0)days.emplace_back(sim);
        Action actions[2];auto oa=agent::runtime::make_observation(sim,0),ob=agent::runtime::make_observation(sim,1);
        own.act(oa,{},actions[0]);rival.act(ob,{},actions[1]);
        validate_action(actions[0],oa);validate_action(actions[1],ob);
        for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
        const auto before=sim;sim.step(actions[0],actions[1]);append_contract(days.back(),before,sim,actions);
    }
    std::ofstream(out/"SOURCE_GAME.json")<<"{\"seed\":1000,\"seat\":0,\"cash\":"<<sim.st.farms[0].money
        <<",\"opponent_cash\":"<<sim.st.farms[1].money<<",\"action_hash\":\""<<hashes[0]<<"\",\"opponent_action_hash\":\""<<hashes[1]<<"\"}\n";
    std::ofstream summary(out/"results.jsonl");
    for(int day:{14,18,22,26}) {
        const auto& source=days[day];
        if(source.discarded){std::cout<<"day="<<day<<" skipped overflow="<<source.discarded<<std::endl;continue;}
        auto prepared=source.problem;day_scheduler::prepare_problem(prepared);
        auto physical=source.own;
        for(auto& action:physical){action.n_orders=0;std::fill(std::begin(action.orders),std::end(action.orders),Order{});}
        for(const auto& event:prepared.market_plan) {
            auto& action=physical[event.hour];action.n_orders=std::max(action.n_orders,int(event.order_index)+1);
            action.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
        }
        for(auto& action:physical)action.finalize();
        auto original=day_solver::replay_schedule(prepared,physical);
        if(!original.requirements_satisfied || !original.invariants_satisfied) {
            std::cerr<<"source contract replay failed day="<<day<<'\n';return 3;
        }
        for(int remove:{0,1,2}) {
            auto problem=source.problem;auto markets=source.own;
            const int added=add_care(source,problem);
            int saving=0;
            for(int n=0;n<remove;++n) {
                saving+=fib(problem.worker_count-2);
                const auto hire=std::find_if(problem.market_plan.rbegin(),problem.market_plan.rend(),[](const auto& e){return e.market_op==M_HIRE;});
                if(hire==problem.market_plan.rend() || hire->quantity!=1)std::abort();
                markets[hire->hour].orders[hire->order_index]={};
                problem.market_plan.erase(std::next(hire).base());--problem.worker_count;
            }
            const auto folder=out/("day"+std::to_string(day)+"_remove"+std::to_string(remove));fs::create_directories(folder);
            day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/"problem.json");save_actions(source.own,folder/"source_schedule.txt");
            day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
            const auto result=day_scheduler::solve(problem,options);
            bool exact=false,cash_exact=false;int faults=0;
            if(result.schedule) {
                auto actions=*result.schedule;auto full=source.start;
                for(int h=0;h<24;++h) {
                    std::copy_n(shops.begin(),full.st.n_shops,full.st.shops);
                    actions[h].n_orders=markets[h].n_orders;
                    std::copy_n(markets[h].orders,markets[h].n_orders,actions[h].orders);actions[h].finalize();
                    validate_action(actions[h],agent::runtime::make_observation(full,0));
                    auto diagnosis=full.diagnose_joint_actions(actions[h],source.rival[h]);
                    faults+=diagnosis.players[0].requested_unit_actions-diagnosis.players[0].successful_unit_actions;
                    full.step(actions[h],source.rival[h]);
                }
                const auto& farm=full.st.farms[0];const auto& expected=days[day+1].start.st.farms[0];
                exact=farm.n_quadrants==expected.n_quadrants && faults==0;
                for(int i=0;i<N_ITEMS;++i)exact&=farm.shed[i]==expected.shed[i] && farm.produced[i]==expected.produced[i] && farm.discarded[i]==expected.discarded[i];
                for(int i=0;i<N_CROPS;++i)exact&=farm.seeds[i]==expected.seeds[i];
                for(const auto& end:problem.required_end_tiles) {
                    auto actual=managed(farm.tiles[end.tile/10][end.tile%10],day+1);const auto desired=*end.exact_state;
                    if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                    exact&=actual==desired;
                }
                cash_exact=farm.money==expected.money+saving && full.st.farms[1].money==days[day+1].start.st.farms[1].money;
                save_actions(actions,folder/"combined_schedule.txt");
            }
            summary<<"{\"day\":"<<day<<",\"remove_hires\":"<<remove<<",\"workers\":"<<problem.worker_count<<",\"added_care\":"<<added
                <<",\"hire_saving\":"<<saving<<",\"solved\":"<<(result.schedule?"true":"false")<<",\"seconds\":"<<result.seconds
                <<",\"full_endpoint_equal\":"<<(exact?"true":"false")<<",\"cash_equal\":"<<(cash_exact?"true":"false")<<",\"faults\":"<<faults<<"}\n";summary.flush();
            std::cout<<"day="<<day<<" remove="<<remove<<" added_care="<<added<<" solved="<<bool(result.schedule)<<" endpoint="<<exact<<" cash="<<cash_exact<<std::endl;
        }
    }
}
