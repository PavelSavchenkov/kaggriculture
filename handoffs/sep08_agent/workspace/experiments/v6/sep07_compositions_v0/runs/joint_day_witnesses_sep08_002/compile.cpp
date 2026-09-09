#include "../../include/day_contract.hpp"
#include "../joint_day_routes_sep08_001/proposals/joint_routes_p355_m0/source/agent.hpp"
#include "../joint_day_routes_sep08_001/proposals/joint_routes_p355_m1/source/agent.hpp"
#include "../joint_day_routes_sep08_001/proposals/joint_routes_p362_m0/source/agent.hpp"
#include "../joint_day_routes_sep08_001/proposals/joint_routes_p362_m1/source/agent.hpp"

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

struct ExtraWork {
    std::array<int,18> actions{};
    std::array<int,N_ITEMS> produced{},inputs{};
    int skipped_construction=0;
};

bool service_op(int op) {
    return op==OP_FEED || op==OP_CARE || op==OP_COLLECT_FERTILIZER ||
        op==OP_WATER || op==OP_FERTILIZE || op==OP_HARVEST;
}

// This is offline contract construction. Virtual input injections compute tile
// semantics only; all consumed inputs are deducted from the real day contract.
// The scheduler receives no virtual route or unlimited stock.
template<class Agent> ExtraWork add_pending(const RecordedDay& source,const Sim& before_night,
        const Agent& policy,DayProblem& problem) {
    ExtraWork extra;
    auto semantic=before_night;
    const auto market=semantic.st.market;
    Agent shadow=policy;
    for(int round=0;round<8;++round) {
        semantic.st.step=source.start.st.day*24+23;
        semantic.st.day=source.start.st.day;semantic.st.hour=23;semantic.st.market=market;
        auto observation=agent::runtime::make_observation(semantic,0);
        typename Agent::MarketContext context;Action unused;
        shadow.plan_units(observation,unused,context);
        int added=0,skipped=0;
        for(int cell=0;cell<100;++cell)for(int j=0;j<context.task_count[cell];++j) {
            const auto& task=context.tasks[cell][j];
            if(!service_op(task.action.op)){++skipped;continue;}
            auto& farm=semantic.st.farms[0];
            farm.pos_x[0]=cell%10;farm.pos_y[0]=cell/10;
            if(task.input>=0){farm.inv_add(0,task.input,1);++extra.inputs[task.input];}
            Action actions[2];
            for(int p=0;p<2;++p){actions[p].clear();actions[p].n_units=semantic.st.farms[p].n_units;}
            actions[0].units[0]=task.action;
            for(auto& action:actions)action.finalize();
            semantic.st.step=source.start.st.day*24+23;semantic.st.hour=0;semantic.st.market=market;
            const auto diagnosis=semantic.diagnose_joint_actions(actions[0],actions[1]);
            if(diagnosis.players[0].successful_unit_actions!=diagnosis.players[0].requested_unit_actions)std::abort();
            std::array<int64_t,N_ITEMS> produced;
            std::copy_n(farm.produced,N_ITEMS,produced.begin());
            const int harvested_item=farm.tiles[cell/10][cell%10].what;
            semantic.step(actions[0],actions[1]);
            auto found=std::find_if(problem.tile_work.begin(),problem.tile_work.end(),[&](const auto& work){return work.tile==cell;});
            if(found==problem.tile_work.end()){problem.tile_work.push_back({int16_t(cell),{}});found=problem.tile_work.end()-1;}
            TileWorkAction work;work.op=task.action.op;
            for(int item=0;item<N_ITEMS;++item) {
                const int quantity=semantic.st.farms[0].produced[item]-produced[item];
                extra.produced[item]+=quantity;
                if(quantity){work.output_item=item;work.output_quantity=quantity;}
            }
            if(work.op==OP_HARVEST) {
                work.arg=work.output_item;
                if(work.output_item<0 || work.output_quantity<=0 || harvested_item<0)std::abort();
            }
            found->actions.push_back(work);++extra.actions[work.op];++added;
        }
        extra.skipped_construction=skipped;
        if(!added)break;
        if(round==7)std::abort();
    }
    const auto daytime=semantic.st.farms[0];
    Action pass[2];
    for(int p=0;p<2;++p){pass[p].clear();pass[p].n_units=semantic.st.farms[p].n_units;pass[p].finalize();}
    semantic.st.step=source.start.st.day*24+23;semantic.st.day=source.start.st.day;semantic.st.hour=23;
    semantic.step(pass[0],pass[1]);
    for(auto& end:problem.required_end_tiles) {
        const int cell=end.tile;auto tile=semantic.st.farms[0].tiles[cell/10][cell%10];
        if(tile.kind==T_WEED && daytime.tiles[cell/10][cell%10].kind==T_EMPTY)tile={};
        end.exact_state=managed(tile,source.start.st.day+1);
    }
    for(int item=0;item<N_ITEMS;++item)problem.end_shed[item]+=extra.produced[item]-extra.inputs[item];
    return extra;
}

template<class Agent> void study(const fs::path& root,const std::string& name,double seconds) {
    const auto out=root/name;fs::create_directories(out);
    Config config;config.seed=1000;Sim sim(config);
    Agent own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=1000^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
    for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::vector<RecordedDay> days;days.reserve(30);
    std::vector<Sim> before_night;before_night.reserve(30);
    std::vector<Agent> policies;policies.reserve(30);
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done) {
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        if(sim.st.hour==0)days.emplace_back(sim);
        Action actions[2];auto oa=agent::runtime::make_observation(sim,0),ob=agent::runtime::make_observation(sim,1);
        own.act(oa,{},actions[0]);rival.act(ob,{},actions[1]);
        validate_action(actions[0],oa);validate_action(actions[1],ob);
        for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
        const auto before=sim;
        if(sim.st.hour==23){before_night.push_back(prefix_phase(sim,actions,10));policies.push_back(own);}
        sim.step(actions[0],actions[1]);append_contract(days.back(),before,sim,actions);
    }
    std::ofstream(out/"SOURCE_GAME.json")<<"{\"seed\":1000,\"seat\":0,\"cash\":"<<sim.st.farms[0].money
        <<",\"opponent_cash\":"<<sim.st.farms[1].money<<",\"action_hash\":\""<<hashes[0]<<"\",\"opponent_action_hash\":\""<<hashes[1]
        <<"\",\"day14_start_hash\":\""<<days[14].start.parity_hash()<<"\"}\n";
    std::ofstream summary(out/"results.jsonl");
    for(int day:{16}) {
        const auto& source=days[day];
        if(source.discarded){std::cerr<<name<<" source overflow day="<<day<<'\n';std::abort();}
        auto prepared=source.problem;day_scheduler::prepare_problem(prepared);
        auto physical=source.own;
        for(auto& action:physical){action.n_orders=0;std::fill(std::begin(action.orders),std::end(action.orders),Order{});}
        for(const auto& event:prepared.market_plan) {
            auto& action=physical[event.hour];action.n_orders=std::max(action.n_orders,int(event.order_index)+1);
            action.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
        }
        for(auto& action:physical)action.finalize();
        const auto original=day_solver::replay_schedule(prepared,physical);
        if(!original.requirements_satisfied || !original.invariants_satisfied)std::abort();
        for(int augment:{0,1})for(int remove:{0,2}) {
            if(augment==1 && remove==2)continue;
            auto problem=source.problem;auto markets=source.own;ExtraWork extra;
            if(augment)extra=add_pending(source,before_night[day],policies[day],problem);
            bool resources=std::all_of(problem.end_shed.begin(),problem.end_shed.end(),[](auto n){return n>=0;});
            int saving=0;
            for(int n=0;n<remove;++n) {
                saving+=fib(problem.worker_count-2);
                auto hire=std::find_if(problem.market_plan.rbegin(),problem.market_plan.rend(),[](const auto& e){return e.market_op==M_HIRE;});
                if(hire==problem.market_plan.rend() || hire->quantity!=1)std::abort();
                markets[hire->hour].orders[hire->order_index]={};
                problem.market_plan.erase(std::next(hire).base());--problem.worker_count;
            }
            const auto folder=out/("day"+std::to_string(day)+"_augment"+std::to_string(augment)+"_remove"+std::to_string(remove));
            fs::create_directories(folder);save_actions(source.own,folder/"source_schedule.txt");
            bool solved=false,exact=false,cash_exact=false;int faults=0;double elapsed=0;
            if(resources) {
                day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/"problem.json");
                day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
                const auto result=day_scheduler::solve(problem,options);elapsed=result.seconds;solved=bool(result.schedule);
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
                    for(int i=0;i<N_ITEMS;++i)exact&=farm.shed[i]==problem.end_shed[i] &&
                        farm.produced[i]==expected.produced[i]+extra.produced[i] && farm.discarded[i]==expected.discarded[i];
                    for(int i=0;i<N_CROPS;++i)exact&=farm.seeds[i]==expected.seeds[i];
                    for(const auto& end:problem.required_end_tiles) {
                        auto actual=managed(farm.tiles[end.tile/10][end.tile%10],day+1);const auto desired=*end.exact_state;
                        if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                        exact&=actual==desired;
                    }
                    cash_exact=farm.money==expected.money+saving && full.st.farms[1].money==days[day+1].start.st.farms[1].money;
                    save_actions(actions,folder/"combined_schedule.txt");
                }
            }
            summary<<"{\"day\":"<<day<<",\"augment\":"<<augment<<",\"remove_hires\":"<<remove<<",\"workers\":"<<problem.worker_count
                <<",\"hire_saving\":"<<saving<<",\"resources_nonnegative\":"<<(resources?"true":"false")<<",\"solved\":"<<(solved?"true":"false")
                <<",\"seconds\":"<<elapsed<<",\"full_endpoint_equal\":"<<(exact?"true":"false")<<",\"cash_equal\":"<<(cash_exact?"true":"false")
                <<",\"faults\":"<<faults<<",\"added_actions\":[";
            for(int i=0;i<18;++i){if(i)summary<<',';summary<<extra.actions[i];}
            summary<<"],\"added_produced\":[";for(int i=0;i<N_ITEMS;++i){if(i)summary<<',';summary<<extra.produced[i];}
            summary<<"],\"extra_inputs\":[";for(int i=0;i<N_ITEMS;++i){if(i)summary<<',';summary<<extra.inputs[i];}
            summary<<"],\"skipped_construction\":"<<extra.skipped_construction<<"}\n";summary.flush();
            std::cout<<name<<" day="<<day<<" augment="<<augment<<" remove="<<remove<<" resources="<<resources<<" solved="<<solved
                <<" exact="<<exact<<" cash="<<cash_exact<<std::endl;
        }
    }
}

int main(int argc,char** argv) {
    if(argc!=3)return 2;fs::path out=argv[1];double seconds=std::stod(argv[2]);
    if(fs::exists(out) || seconds<=0)return 2;fs::create_directories(out);
    study<joint_routes_p362_m1::Agent>(out,"joint_routes_p362_m1",seconds);
}
