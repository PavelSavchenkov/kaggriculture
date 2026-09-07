#include "../include/day_contract.hpp"
#include "../include/day_override.hpp"
#include <cctype>
#include <iomanip>

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

struct SourceDay {
    RecordedDay recorded;
    std::array<std::array<int,9>,24> inventory;
    std::array<double,2> end_cash{};
    std::array<int64_t,N_ITEMS> end_produced{};
    int end_land=0;
    bool usable=false;
    explicit SourceDay(const Sim& sim):recorded(sim) {}
};

std::vector<SourceDay> record_source(int program) {
    top_replay_library::Agent agent(program);public_router::Agent rival;
    Config config;config.seed=1000;Sim sim(config);
    agent.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=1000^0xa37108e62d045fb9ULL;
    std::array<uint8_t,8> shops;for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::vector<SourceDay> days;days.reserve(29);
    for(int day=0;day<29;++day) {
        days.emplace_back(sim);auto& source=days.back();
        for(int hour=0;hour<24;++hour) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            std::copy_n(sim.st.market.inventory,9,source.inventory[hour].begin());
            Action actions[2];
            agent.act(agent::runtime::make_observation(sim,0),{},actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),{},actions[1]);
            const auto before=sim;sim.step(actions[0],actions[1]);
            append_contract(source.recorded,before,sim,actions);
        }
        for(int p=0;p<2;++p)source.end_cash[p]=sim.st.farms[p].money;
        std::copy_n(sim.st.farms[0].produced,N_ITEMS,source.end_produced.begin());
        source.end_land=sim.st.farms[0].n_quadrants;
        if(source.recorded.discarded)continue;
        day_scheduler::prepare_problem(source.recorded.problem);
        source.usable=true;
    }
    return days;
}

struct SaleEdit {int day,old_hour,slot,new_hour,item,quantity;double estimate;};

double sell_value(int item,int inventory,int quantity) {
    double value=0;
    for(int n=0;n<quantity;++n) {
        const int price=market_price(item,inventory);value+=price;if(price>1)++inventory;
    }
    return value;
}

std::vector<SaleEdit> propose_edits(const std::vector<SourceDay>& days) {
    std::vector<SaleEdit> edits;
    for(int day=0;day<29;++day) {
        const auto& source=days[day];if(!source.usable)continue;
        for(int hour=1;hour<24;++hour) {
            const auto& action=source.recorded.own[hour];
            for(int slot=0;slot<action.n_orders;++slot) {
                const auto order=action.orders[slot];
                if(order.op!=M_SELL || order.item<1 || order.item>7 || order.n<=0)continue;
                for(int earlier=std::max(0,hour-4);earlier<hour;++earlier) {
                    const auto& prior=source.recorded.own[earlier];
                    if(prior.n_orders>=10)continue;
                    bool duplicate=false;
                    for(int i=0;i<prior.n_orders;++i)duplicate|=prior.orders[i].op==M_SELL && prior.orders[i].item==order.item;
                    if(duplicate)continue;
                    const double value=sell_value(order.item,source.inventory[earlier][order.item],order.n)
                        -sell_value(order.item,source.inventory[hour][order.item],order.n);
                    if(value>0)edits.push_back({day,hour,slot,earlier,order.item,order.n,value});
                }
            }
        }
    }
    std::stable_sort(edits.begin(),edits.end(),[](const SaleEdit& a,const SaleEdit& b){return a.estimate>b.estimate;});
    return edits;
}

void export_agent(const std::string& name,int program,const SaleEdit& edit,
                  const std::array<Action,24>& actions,const fs::path& directory) {
    fs::create_directories(directory/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    const auto include=fs::relative(experiment/"include/day_override.hpp",directory/"source").generic_string();
    const auto implementation=fs::relative(experiment/"league/top_replay_library/source/agent.cpp",directory).generic_string();
    std::ofstream header(directory/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<include<<"\"\nnamespace compositions::"<<name<<" {\ninline std::array<kag::Action,24> schedule() {\n"
        <<"constexpr int data[]={\n";
    for(const auto& action:actions) {
        header<<action.n_units<<','<<action.n_orders<<',';
        for(int u=0;u<action.n_units;++u)header<<+action.units[u].op<<','<<+action.units[u].arg<<','<<action.units[u].n<<',';
        for(int i=0;i<action.n_orders;++i)header<<+action.orders[i].op<<','<<+action.orders[i].item<<','<<action.orders[i].n<<',';
        header<<'\n';
    }
    header<<"};\nstd::array<kag::Action,24> result;const int* p=data;\nfor(auto& action:result){action.n_units=*p++;action.n_orders=*p++;\n"
        <<"for(int u=0;u<action.n_units;++u){action.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}\n"
        <<"for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}action.finalize();}return result;}\n"
        <<"class Agent:public DayOverrideAgent {public:Agent():DayOverrideAgent("<<program<<','<<edit.day<<",schedule()){}\n"
        <<"static kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};\n}\n";
    std::ofstream cpp(directory/"source/agent.cpp");cpp<<"#include \"agent.hpp\"\n";
    std::ofstream manifest(directory/"agent.json");manifest<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"
        <<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<implementation<<"\"]}\n";
    std::ofstream readme(directory/"README.md");
    readme<<"# "<<name<<"\n\nLocal day-schedule reconstruction of top_replay_library program "<<program
        <<"; exact replay/episode/seat/submission provenance remains in that library's IMPORT.json. "
        <<"On day "<<edit.day<<", move sale of "<<edit.quantity<<" units of item "<<edit.item<<" from hour "<<edit.old_hour<<" to "<<edit.new_hour
        <<". Other field work, input purchases and day endpoints are preserved as typed constraints; V30 rebuilds all worker routes for that day. "
        <<"The contract comes from discovery seed 1000 seat 0 versus public_router, using observed successful quantities. "
        <<"Runtime is a complete fixed course with normalized worker counts; no seed, identity or hidden input is accessed. "
        <<"Compiler code is local; V30 is the persistent repository solver.\n\n"
        <<"Experimental discovery artifact, not promoted. Exact full-game results and local failures are retained under the same run name. "
        <<"A one-day gain against recorded rival actions does not prove full-game or general improvement.\n";
}

int main(int argc,char** argv) {
    if(argc!=5) {std::cerr<<"usage: advance_sales PROGRAM RUN_DIRECTORY MAX_CANDIDATES SECONDS_PER_DAY\n";return 2;}
    const int program=std::stoi(argv[1]),limit=std::stoi(argv[3]);const double seconds=std::stod(argv[4]);
    const fs::path directory=argv[2];const std::string run=directory.filename();
    if(limit<=0 || seconds<=0 || run.empty())return 2;
    for(char c:run)if(!(std::isalnum(static_cast<unsigned char>(c)) || c=='_'))return 2;
    if(fs::exists(directory) && !fs::is_empty(directory)) {std::cerr<<"Refusing to overwrite existing run\n";return 2;}
    fs::create_directories(directory/"exact");fs::create_directories(directory/"proposals");
    auto days=record_source(program);auto edits=propose_edits(days);
    std::ofstream log(directory/"edits.csv");
    log<<"id,day,item,quantity,old_hour,new_hour,estimated_gain,solved,seconds,physical_equal,cash_gain,margin_gain\n";
    for(int id=0;id<std::min(limit,int(edits.size()));++id) {
        const auto edit=edits[id];const auto& source=days[edit.day];
        auto problem=source.recorded.problem;auto market=source.recorded.own;
        auto sale=market[edit.old_hour].orders[edit.slot];market[edit.old_hour].orders[edit.slot]={};
        auto& destination=market[edit.new_hour];destination.orders[destination.n_orders++]=sale;
        for(int hour=edit.new_hour;hour<edit.old_hour;++hour)problem.shed_availability[hour][edit.item]+=edit.quantity;
        const std::string name=run+"_"+std::to_string(id);const auto folder=directory/"proposals"/name;
        fs::create_directories(folder);day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/"problem.json");
        day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
        const auto result=day_scheduler::solve(problem,options);
        bool equal=false;double gain=0,margin=0;
        if(result.schedule) {
            auto actions=*result.schedule;auto rebuilt=source.recorded.start;
            uint64_t random=1000^0xa37108e62d045fb9ULL;
            std::array<uint8_t,8> shops;for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
            for(int hour=0;hour<24;++hour) {
                std::copy_n(shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                actions[hour].n_orders=market[hour].n_orders;
                std::copy_n(market[hour].orders,market[hour].n_orders,actions[hour].orders);actions[hour].finalize();
                rebuilt.step(actions[hour],source.recorded.rival[hour]);
            }
            const auto& farm=rebuilt.st.farms[0];equal=farm.n_quadrants==source.end_land;
            for(int item=0;item<N_ITEMS;++item)equal&=farm.shed[item]==problem.end_shed[item] && farm.produced[item]==source.end_produced[item]
                && farm.discarded[item]==source.recorded.start.st.farms[0].discarded[item];
            for(int item=0;item<N_CROPS;++item)equal&=farm.seeds[item]==problem.end_seeds[item];
            for(int cell=0;cell<100;++cell) {
                const auto expected=*problem.required_end_tiles[cell].exact_state;
                auto actual=managed(farm.tiles[cell/10][cell%10],edit.day+1);
                if(expected.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                equal&=actual==expected;
            }
            gain=farm.money-source.end_cash[0];
            margin=gain-(rebuilt.st.farms[1].money-source.end_cash[1]);
            save_actions(actions,folder/"combined_schedule.txt");
            if(equal) {
                export_agent(name,program,edit,actions,folder);
                compositions::Options exact;exact.a=name;exact.b="public_router";exact.validate=true;exact.threads=4;
                exact.output=(directory/"exact"/(name+".json")).string();exact.seeds={1000,1001,1002,1003};exact.seat_mode=2;
                run_batch(exact,[&]{return DayOverrideAgent(program,edit.day,actions);},[]{return public_router::Agent{};});
            }
        }
        log<<id<<','<<edit.day<<','<<edit.item<<','<<edit.quantity<<','<<edit.old_hour<<','<<edit.new_hour<<','<<edit.estimate
            <<','<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<','<<gain<<','<<margin<<'\n';log.flush();
        std::cout<<"edit="<<id<<" day="<<edit.day<<" solved="<<bool(result.schedule)<<" equal="<<equal<<" cash_gain="<<gain<<" margin_gain="<<margin<<std::endl;
    }
    std::cout<<"proposed="<<edits.size()<<" selected="<<std::min(limit,int(edits.size()))<<std::endl;
}
