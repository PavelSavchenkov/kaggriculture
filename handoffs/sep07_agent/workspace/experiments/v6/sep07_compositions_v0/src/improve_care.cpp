#include "../include/day_contract.hpp"
#include "../include/day_override.hpp"
#include <cctype>

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

struct Season {
    std::vector<RecordedDay> days;
    std::array<std::array<Action,2>,719> actions;
    std::array<uint8_t,8> shops;
    Sim finish;
    explicit Season(int program):finish(Config{}) {
        Config config;config.seed=1000;Sim sim(config);
        top_replay_library::Agent own(program);public_router::Agent rival;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        uint64_t random=1000^0xa37108e62d045fb9ULL;
        for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
        days.reserve(30);
        for(int step=0;step<719;++step) {
            if(sim.st.hour==0)days.emplace_back(sim);
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            own.act(agent::runtime::make_observation(sim,0),{},actions[step][0]);
            rival.act(agent::runtime::make_observation(sim,1),{},actions[step][1]);
            const auto before=sim;sim.step(actions[step][0],actions[step][1]);
            Action pair[2]={actions[step][0],actions[step][1]};
            append_contract(days.back(),before,sim,pair);
        }
        finish=sim;
        for(int day=0;day<29;++day)if(!days[day].discarded)day_scheduler::prepare_problem(days[day].problem);
    }
};

const TileWork* work_at(const RecordedDay& day,int cell) {
    const auto& work=day.problem.tile_work;
    const auto found=std::find_if(work.begin(),work.end(),[&](const TileWork& w){return w.tile==cell;});
    return found==work.end()?nullptr:&*found;
}

struct CareEdit {
    int day,cell,animal,extra_output=0;
    int bank_delta=1,hire_saving=0;
    bool remove=false;
    double biological_value=0,forecast_cash=0,forecast_margin=0;
    int forecast_output=0,forecast_sold=0,forecast_discarded=0,forecast_shed=0;
};

// Isolated dated animal simulation under the successful source service calendar.
// No movement, funding or sales are assumed feasible here. Every baseline
// harvest and morning state is checked against the full source game.
int marginal_output(const Season& season,const CareEdit& edit) {
    const auto& start=season.days[edit.day+1].start.st.farms[0].tiles[edit.cell/10][edit.cell%10];
    Tile tiles[2]={start,start};tiles[1].pending_care_bonus+=edit.bank_delta;
    if(tiles[1].pending_care_bonus<0)std::abort();
    int output[2]={};
    for(int day=edit.day+1;day<30;++day) {
        const auto* work=work_at(season.days[day],edit.cell);
        if(work)for(const auto& action:work->actions) {
            for(int variant=0;variant<2;++variant) {
                auto& tile=tiles[variant];
                if(action.op==OP_FEED)tile.fed_today=true;
                if(action.op==OP_CARE)tile.cared_today=true;
                if(action.op==OP_COLLECT_FERTILIZER)tile.fertilizer_available=false;
                if(action.op==OP_HARVEST) {
                    if(variant==0 && tile.yield_units!=action.output_quantity)std::abort();
                    output[variant]+=tile.yield_units;tile.yield_units=0;
                }
            }
        }
        if(day==29)break;
        for(auto& tile:tiles) {
            tile.consecutive_dry=tile.fed_today?0:tile.consecutive_dry+1;
            if(tile.consecutive_dry>=2) {tile.has_animal=false;continue;}
            const auto definition=ANIMALS[tile.what-GOOSE];
            const int since=day+1-tile.planted_day-definition.first_yield_day;
            if(since>=0 && since%definition.interval==0) {
                tile.yield_units=std::min(definition.max_held,tile.yield_units+1+(tile.fed_today?tile.pending_care_bonus:0));
                tile.pending_care_bonus=0;
            }
            if(tile.fed_today && tile.cared_today)++tile.pending_care_bonus;
            tile.fertilizer_available=true;tile.fed_today=tile.cared_today=false;
        }
        if(!tiles[0].has_animal)break;
        const auto& expected=season.days[day+1].start.st.farms[0].tiles[edit.cell/10][edit.cell%10];
        if(managed(tiles[0],day+1)!=managed(expected,day+1))std::abort();
    }
    return output[1]-output[0];
}

void forecast(const Season& season,CareEdit& edit) {
    Sim simulated=season.days[edit.day+1].start;
    simulated.st.farms[0].tiles[edit.cell/10][edit.cell%10].pending_care_bonus+=edit.bank_delta;
    for(int step=(edit.day+1)*24;step<719;++step) {
        std::copy_n(season.shops.begin(),simulated.st.n_shops,simulated.st.shops);
        simulated.step(season.actions[step][0],season.actions[step][1]);
    }
    const auto& original=season.finish.st.farms[0];const auto& changed=simulated.st.farms[0];
    const int product=ANIMALS[edit.animal-GOOSE].product;
    edit.forecast_cash=changed.money-original.money;
    edit.forecast_margin=edit.forecast_cash-(simulated.st.farms[1].money-season.finish.st.farms[1].money);
    edit.forecast_output=changed.produced[product]-original.produced[product];
    edit.forecast_sold=changed.sold_units[product]-original.sold_units[product];
    edit.forecast_discarded=changed.discarded[product]-original.discarded[product];
    edit.forecast_shed=changed.shed[product]-original.shed[product];
}

void export_agent(const std::string& name,int program,const CareEdit& edit,
                  const std::array<Action,24>& actions,const fs::path& directory) {
    fs::create_directories(directory/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    const auto include=fs::relative(experiment/"include/day_override.hpp",directory/"source").generic_string();
    const auto implementation=fs::relative(experiment/"league/top_replay_library/source/agent.cpp",directory).generic_string();
    std::ofstream header(directory/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<include<<"\"\nnamespace compositions::"<<name<<" {\ninline std::array<kag::Action,24> schedule() {\nconstexpr int data[]={\n";
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
    std::ofstream(directory/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream(directory/"agent.json")<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"
        <<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<implementation<<"\"]}\n";
    std::ofstream(directory/"README.md")<<"# "<<name<<"\n\nSource course "<<program<<" from the experiment's top_replay_library; exact source lineage remains in its IMPORT.json. "
        <<(edit.cell<0?"Remove the final daily hire with original services":edit.remove?"Remove CARE and the final daily hire":"Add CARE")<<" on day "<<edit.day
        <<"; selected tile index "<<edit.cell<<" (-1 means no service edit). Preserve all other day work, end stocks and economic orders. "
        <<"V30 rebuilds the complete worker day. Local code proposes and estimates the service edit. "
        <<"Contract from discovery seed 1000 seat 0 versus public_router. No seed or hidden input is used at runtime. "
        <<"Fixed complete course with normalized worker counts; discovery artifact, not promoted. Full later dependencies are tested in live-opponent games.\n";
}

int main(int argc,char** argv) {
    if(argc!=5 && argc!=6) {std::cerr<<"usage: improve_care PROGRAM RUN_DIRECTORY MAX_CANDIDATES SECONDS_PER_DAY [add|remove|hire]\n";return 2;}
    const std::string mode=argc==6?argv[5]:"add";if(mode!="add" && mode!="remove" && mode!="hire")return 2;
    const bool remove=mode=="remove";
    const bool reduce_hire=mode!="add";
    const int program=std::stoi(argv[1]),limit=std::stoi(argv[3]);const double seconds=std::stod(argv[4]);
    const fs::path directory=argv[2];const std::string run=directory.filename();
    if(limit<=0 || seconds<=0 || run.empty())return 2;
    for(char c:run)if(!(std::isalnum(static_cast<unsigned char>(c)) || c=='_'))return 2;
    if(fs::exists(directory) && !fs::is_empty(directory)) {std::cerr<<"Refusing to overwrite existing run\n";return 2;}
    fs::create_directories(directory/"proposals");fs::create_directories(directory/"exact");
    const Season season(program);std::vector<CareEdit> edits;
    const auto begin=std::chrono::steady_clock::now();
    for(int day=0;day<29;++day) {
        if(season.days[day].discarded)continue;
        if(mode=="hire") {
            if(season.days[day].problem.worker_count<=1)continue;
            CareEdit edit{day,-1,-1};edit.bank_delta=0;
            edit.hire_saving=season.days[day].start.cfg.hire_mult*fib(season.days[day].problem.worker_count-2);
            edits.push_back(edit);continue;
        }
        for(const auto& work:season.days[day].problem.tile_work) {
            bool fed=false,cared=false;
            for(const auto& a:work.actions) {fed|=a.op==OP_FEED;cared|=a.op==OP_CARE;}
            const auto& tile=season.days[day+1].start.st.farms[0].tiles[work.tile/10][work.tile%10];
            if(!tile.has_animal || (remove?!cared:(!fed || cared)))continue;
            CareEdit edit{day,work.tile,tile.what};edit.remove=remove;edit.bank_delta=remove?(fed?-1:0):1;
            if(remove) {
                if(season.days[day].problem.worker_count<=1)continue;
                edit.hire_saving=season.days[day].start.cfg.hire_mult*fib(season.days[day].problem.worker_count-2);
            }
            edit.extra_output=marginal_output(season,edit);
            const int product=ANIMALS[tile.what-GOOSE].product;
            edit.biological_value=edit.extra_output*market_price(product,season.days[day].start.st.market.inventory[product]);
            edits.push_back(edit);
        }
    }
    const double estimate_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    const int considered=edits.size();const auto forecast_start=std::chrono::steady_clock::now();
    std::ofstream estimates(directory/"estimates.csv");
    estimates<<"day,cell,animal,bank_delta,hire_saving,extra_output,biological_value,forecast_cash,forecast_margin,forecast_output,forecast_sold,forecast_discarded,forecast_shed\n";
    for(auto& edit:edits) {
        if(edit.cell>=0)forecast(season,edit);
        estimates<<edit.day<<','<<edit.cell<<','<<edit.animal<<','<<edit.bank_delta<<','<<edit.hire_saving<<','<<edit.extra_output<<','<<edit.biological_value<<','<<edit.forecast_cash<<','<<edit.forecast_margin
            <<','<<edit.forecast_output<<','<<edit.forecast_sold<<','<<edit.forecast_discarded<<','<<edit.forecast_shed<<'\n';
    }
    estimates.close();
    const double forecast_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-forecast_start).count();
    std::erase_if(edits,[&](const CareEdit& e){return reduce_hire?(e.extra_output!=0 || e.forecast_output!=0 || e.forecast_cash<0):e.extra_output<=0;});
    std::stable_sort(edits.begin(),edits.end(),[](const CareEdit& a,const CareEdit& b) {
        return std::tuple(a.forecast_margin+a.hire_saving,a.forecast_cash,a.biological_value)>
               std::tuple(b.forecast_margin+b.hire_saving,b.forecast_cash,b.biological_value);
    });
    std::ofstream log(directory/"compiled.csv");
    log<<"id,day,cell,extra_output,forecast_cash,solved,seconds,endpoint_equal,cash_equal\n";
    for(int id=0;id<std::min(limit,int(edits.size()));++id) {
        const auto& edit=edits[id];
        const auto& source=season.days[edit.day];auto problem=source.problem;auto markets=source.own;
        auto found=std::find_if(problem.tile_work.begin(),problem.tile_work.end(),[&](const TileWork& w){return w.tile==edit.cell;});
        if(remove) {
            std::erase_if(found->actions,[](const TileWorkAction& a){return a.op==OP_CARE;});
            if(found->actions.empty())problem.tile_work.erase(found);
        } else if(mode=="add") {TileWorkAction care;care.op=OP_CARE;found->actions.push_back(care);}
        if(reduce_hire) {
            const auto hire=std::find_if(problem.market_plan.rbegin(),problem.market_plan.rend(),[](const MarketEvent& e){return e.market_op==M_HIRE;});
            if(hire==problem.market_plan.rend() || hire->quantity!=1)std::abort();
            markets[hire->hour].orders[hire->order_index]={};
            problem.market_plan.erase(std::next(hire).base());--problem.worker_count;
        }
        if(edit.cell>=0)problem.required_end_tiles[edit.cell].exact_state->pending_care_bonus+=edit.bank_delta;
        const std::string name=run+"_"+std::to_string(id);const auto folder=directory/"proposals"/name;
        fs::create_directories(folder);day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/"problem.json");
        day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
        const auto result=day_scheduler::solve(problem,options);bool equal=false,cash_equal=false;
        if(result.schedule) {
            auto actions=*result.schedule;auto rebuilt=source.start;
            for(int hour=0;hour<24;++hour) {
                std::copy_n(season.shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                actions[hour].n_orders=markets[hour].n_orders;
                std::copy_n(markets[hour].orders,markets[hour].n_orders,actions[hour].orders);actions[hour].finalize();
                rebuilt.step(actions[hour],source.rival[hour]);
            }
            const auto& farm=rebuilt.st.farms[0];const auto& expected=season.days[edit.day+1].start.st.farms[0];
            equal=farm.n_quadrants==expected.n_quadrants;
            for(int i=0;i<N_ITEMS;++i)equal&=farm.shed[i]==expected.shed[i] && farm.produced[i]==expected.produced[i] && farm.discarded[i]==expected.discarded[i];
            for(int i=0;i<N_CROPS;++i)equal&=farm.seeds[i]==expected.seeds[i];
            for(int cell=0;cell<100;++cell) {
                auto actual=managed(farm.tiles[cell/10][cell%10],edit.day+1);const auto desired=*problem.required_end_tiles[cell].exact_state;
                if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                equal&=actual==desired;
            }
            cash_equal=farm.money==expected.money+edit.hire_saving && rebuilt.st.farms[1].money==season.days[edit.day+1].start.st.farms[1].money;
            save_actions(actions,folder/"combined_schedule.txt");
            if(equal && cash_equal) {
                export_agent(name,program,edit,actions,folder);
                compositions::Options exact;exact.a=name;exact.b="public_router";exact.validate=true;exact.threads=4;
                exact.output=(directory/"exact"/(name+".json")).string();exact.seeds={1000,1001,1002,1003};exact.seat_mode=2;
                run_batch(exact,[&]{return DayOverrideAgent(program,edit.day,actions);},[]{return public_router::Agent{};});
            }
        }
        log<<id<<','<<edit.day<<','<<edit.cell<<','<<edit.extra_output<<','<<edit.forecast_cash<<','<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<','<<cash_equal<<'\n';log.flush();
        std::cout<<"edit="<<id<<" day="<<edit.day<<" cell="<<edit.cell<<" solved="<<bool(result.schedule)<<" equal="<<equal<<" cash_equal="<<cash_equal<<std::endl;
    }
    std::ofstream(directory/"timing.json")<<"{\"considered\":"<<considered<<",\"eligible\":"<<edits.size()<<",\"biology_seconds\":"<<estimate_seconds<<",\"forecast_seconds\":"<<forecast_seconds<<"}\n";
    std::cout<<"proposed="<<edits.size()<<" biology_seconds="<<estimate_seconds<<std::endl;
}
