#include "../include/day_contract.hpp"
#include "../runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

void export_schedule(const std::string& name,const std::array<Action,24>& actions,const fs::path& folder) {
    std::ofstream header(folder/"schedule.hpp");
    header<<"#pragma once\n#include \"agents/common/api/agent_api.hpp\"\n#include <array>\nnamespace compositions::"<<name
        <<" {inline std::array<kag::Action,24> schedule(){constexpr int data[]={\n";
    for(const auto& action:actions) {
        header<<action.n_units<<','<<action.n_orders<<',';
        for(int u=0;u<action.n_units;++u)header<<+action.units[u].op<<','<<+action.units[u].arg<<','<<action.units[u].n<<',';
        for(int i=0;i<action.n_orders;++i)header<<+action.orders[i].op<<','<<+action.orders[i].item<<','<<action.orders[i].n<<',';
        header<<'\n';
    }
    header<<"};std::array<kag::Action,24> result;const int* p=data;\nfor(auto& action:result){action.n_units=*p++;action.n_orders=*p++;"
        <<"for(int u=0;u<action.n_units;++u){action.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}"
        <<"for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}action.finalize();}return result;}}\n";
}

int main(int argc,char** argv) {
    if(argc!=4){std::cerr<<"usage: rebuild_investment_days NEW_RUN SEED SECONDS_PER_DAY\n";return 2;}
    const fs::path directory=fs::absolute(argv[1]);const auto seed=std::stoull(argv[2]);const double seconds=std::stod(argv[3]);
    const std::string run=directory.filename();
    if(fs::exists(directory) || seconds<=0 || run.empty())return 2;
    for(char c:run)if(!std::isalnum(static_cast<unsigned char>(c)) && c!='_')return 2;
    fs::create_directories(directory/"proposals");
    Config config;config.seed=seed;Sim sim(config);animal_adaptive_r1_c0_b0::Agent own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    std::array<uint8_t,8> shops;uint64_t random=seed^0xa37108e62d045fb9ULL;
    for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::vector<RecordedDay> days;days.reserve(30);
    for(int step=0;step<719;++step) {
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        if(sim.st.hour==0)days.emplace_back(sim);
        Action pair[2];own.act(agent::runtime::make_observation(sim,0),{},pair[0]);rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
        validate_action(pair[0],agent::runtime::make_observation(sim,0));
        const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
    }
    if(own.chosen()!=SHEEP || own.entry_day()!=11){std::cerr<<"seed does not activate requested investment\n";return 3;}
    std::ofstream log(directory/"compiled.csv");log<<"id,day,hire_saving,solved,seconds,endpoint_equal,cash_equal\n";
    // Day11 contains the choice; days12+ have an observed changed animal.
    for(int day=12;day<29;++day) {
        const auto& source=days[day];if(source.discarded || source.problem.worker_count<=1)continue;
        auto problem=source.problem;auto markets=source.own;
        const int saving=source.start.cfg.hire_mult*fib(problem.worker_count-2);
        const auto hire=std::find_if(problem.market_plan.rbegin(),problem.market_plan.rend(),[](const MarketEvent& e){return e.market_op==M_HIRE;});
        if(hire==problem.market_plan.rend() || hire->quantity!=1)std::abort();
        markets[hire->hour].orders[hire->order_index]={};problem.market_plan.erase(std::next(hire).base());--problem.worker_count;
        const std::string name=run+"_"+std::to_string(day);const auto folder=directory/"proposals"/name;fs::create_directories(folder);
        day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/"problem.json");
        day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
        const auto result=day_scheduler::solve(problem,options);bool equal=false,cash_equal=false;
        if(result.schedule) {
            auto actions=*result.schedule;auto rebuilt=source.start;
            for(int hour=0;hour<24;++hour) {
                std::copy_n(shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                actions[hour].n_orders=markets[hour].n_orders;
                std::copy_n(markets[hour].orders,markets[hour].n_orders,actions[hour].orders);actions[hour].finalize();
                rebuilt.step(actions[hour],source.rival[hour]);
            }
            const auto& farm=rebuilt.st.farms[0];const auto& expected=days[day+1].start.st.farms[0];
            equal=farm.n_quadrants==expected.n_quadrants;
            for(int i=0;i<N_ITEMS;++i)equal&=farm.shed[i]==expected.shed[i] && farm.produced[i]==expected.produced[i] && farm.discarded[i]==expected.discarded[i];
            for(int i=0;i<N_CROPS;++i)equal&=farm.seeds[i]==expected.seeds[i];
            for(int cell=0;cell<100;++cell) {
                auto actual=managed(farm.tiles[cell/10][cell%10],day+1);const auto desired=*problem.required_end_tiles[cell].exact_state;
                if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                equal&=actual==desired;
            }
            cash_equal=farm.money==expected.money+saving && rebuilt.st.farms[1].money==days[day+1].start.st.farms[1].money;
            save_actions(actions,folder/"combined_schedule.txt");
            if(equal && cash_equal)export_schedule(name,actions,folder);
        }
        log<<day<<','<<day<<','<<saving<<','<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<','<<cash_equal<<'\n';log.flush();
        std::cout<<"day="<<day<<" solved="<<bool(result.schedule)<<" endpoint="<<equal<<" cash="<<cash_equal<<std::endl;
    }
}
