#include "../include/day_contract.hpp"
#include "../include/deferred_animal.hpp"
#include "../include/animal_investment_value.hpp"
#include "../include/market_tape.hpp"
#include "../candidates/shop_herd_guarded_001_best/source/agent.hpp"
#ifdef GENERAL_ANIMAL_ENTRIES
#include "../candidates/investment_context_guarded_001_best/source/agent.hpp"
#endif

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;
#ifdef GENERAL_ANIMAL_ENTRIES
using Source=investment_context_guarded_001_best::Agent;
constexpr const char* source_name="investment_context_guarded_001_best";
#else
using Source=shop_herd_guarded_001_best::Agent;
constexpr const char* source_name="shop_herd_guarded_001_best";
#endif
using Policy=DeferredAnimalAgent<Source>;

GuardedDay guarded(const RecordedDay& source,const DayProblem& problem,const std::array<Action,24>& actions) {
    GuardedDay result;result.plan={source.start.st.day,actions};
    const auto& farm=source.start.st.farms[0];result.quadrants=farm.n_quadrants;
    std::copy_n(farm.shed,N_ITEMS,result.shed.begin());std::copy_n(farm.seeds,N_CROPS,result.seeds.begin());
    for(int cell=0;cell<100;++cell) {
        const auto& t=farm.tiles[cell/10][cell%10];result.tiles[cell]=tile_key(t,source.start.st.day);
        result.check[cell]=t.kind==T_PLANT || t.kind==T_COOP || t.kind==T_PASTURE;
    }
    for(const auto& work:problem.tile_work)result.check[work.tile]=true;
    return result;
}

void export_entry(const fs::path& folder,const AnimalEntry& entry) {
    std::ofstream f(folder/"entry.hpp");const auto& g=entry.day;
    f<<"#pragma once\n#include \"../../../include/deferred_animal.hpp\"\nnamespace compositions::"<<folder.filename().string()<<" {inline AnimalEntry entry(){GuardedDay g;g.plan.day="<<g.plan.day<<";constexpr int data[]={\n";
    for(const auto& a:g.plan.actions) {
        f<<a.n_units<<','<<a.n_orders<<',';
        for(int u=0;u<a.n_units;++u)f<<+a.units[u].op<<','<<+a.units[u].arg<<','<<a.units[u].n<<',';
        for(int i=0;i<a.n_orders;++i)f<<+a.orders[i].op<<','<<+a.orders[i].item<<','<<a.orders[i].n<<',';
        f<<'\n';
    }
    f<<"};const int* p=data;for(auto& a:g.plan.actions){a.n_units=*p++;a.n_orders=*p++;for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}a.finalize();}\ng.tiles={{";
    for(int cell=0;cell<100;++cell){f<<(cell?",":"")<<"{{";for(int i=0;i<12;++i)f<<(i?",":"")<<g.tiles[cell][i];f<<"}}";}f<<"}};\n";
    auto array=[&](const char* name,const auto& data){f<<"g."<<name<<"={";int i=0;for(auto x:data)f<<(i++?",":"")<<int(x);f<<"};\n";};
    array("check",g.check);array("shed",g.shed);array("seeds",g.seeds);f<<"g.quadrants="<<g.quadrants<<";return {"<<entry.item<<",std::move(g)};}}\n";
}

// Offline funding check: keep every previously accepted market quantity and
// fixed purchase while inserting the new obligation at the earliest safe hour.
// A day solver's stock constraints alone cannot prove that purchases are paid.
bool insert_funded_order(const RecordedDay& source,DayProblem& problem,std::array<Action,24>& markets,
                         const std::array<uint8_t,8>& shops,int op,int product,int quantity,
                         int reserved_hour=-1,int reserved_slot=-1) {
    auto accepted=[&](const std::array<Action,24>& actions) {
        auto sim=source.start;std::array<MarketStep,24> result;
        for(int hour=0;hour<24;++hour) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            Action pair[2]={actions[hour],source.rival[hour]};result[hour]=accepted_market(sim,pair);sim.step(pair[0],pair[1]);
        }
        return result;
    };
    const auto baseline=accepted(markets);
    for(int hour=0;hour<23;++hour) {
        int slot=markets[hour].n_orders;
        if(hour==reserved_hour && reserved_slot>=0 && reserved_slot<slot && markets[hour].orders[reserved_slot].op==M_NONE)slot=reserved_slot;
        if(slot>=10)continue;
        auto trial=markets;trial[hour].orders[slot]={uint8_t(op),uint8_t(product),quantity};
        trial[hour].n_orders=std::max(trial[hour].n_orders,slot+1);trial[hour].finalize();
        const auto outcome=accepted(trial);const auto& added=outcome[hour].slots[slot];
        bool paid=op==M_BUY_PRODUCT?added.trades[0].n==quantity:
            op==M_BUY_ANIMAL?added.fixed[0]==ANIMALS[product-GOOSE].cost*quantity:added.fixed[0]>0;
        for(int h=0;h<24 && paid;++h)for(int i=0;i<markets[h].n_orders && paid;++i) {
            if(h==hour && i==slot)continue;
            const auto& expected=baseline[h].slots[i];const auto& actual=outcome[h].slots[i];
            paid=expected.trades[0].n==actual.trades[0].n && expected.fixed[0]==actual.fixed[0];
        }
        if(!paid)continue;
        markets=std::move(trial);problem.market_plan.push_back({int8_t(hour),int8_t(slot),uint8_t(op),int16_t(op==M_HIRE?-1:product),quantity});
        return true;
    }
    return false;
}

int main(int argc,char** argv) {
    if(argc!=4 && argc<9){std::cerr<<"usage: compile_animal_entries NEW_RUN SEED SECONDS_PER_CHOICE [STEP ORDER CELL ORIGINAL DAY...]\n";return 2;}
    const fs::path folder=fs::absolute(argv[1]);const uint64_t seed=std::stoull(argv[2]);const double seconds=std::stod(argv[3]);
    const int purchase_step=argc>4?std::stoi(argv[4]):265,purchase_order=argc>4?std::stoi(argv[5]):7;
    const int animal_cell=argc>4?std::stoi(argv[6]):32,original=argc>4?std::stoi(argv[7]):GOOSE;
    std::vector<int> entry_days{11,12,15,18,21};
    if(argc>4){entry_days.clear();for(int i=8;i<argc;++i)entry_days.push_back(std::stoi(argv[i]));}
    if(purchase_step<0 || purchase_step>=696 || purchase_order<0 || purchase_order>=10 || animal_cell<0 || animal_cell>=100 || !is_animal(original))return 2;
    for(int day:entry_days)if(day<purchase_step/24 || day>=29)return 2;
    if(fs::exists(folder) || seconds<=0)return 2;fs::create_directories(folder);
    Config config;config.seed=seed;Sim sim(config);Policy wait(purchase_step,purchase_order,animal_cell,{},30,GOOSE,{},true,0,0,original);public_router::Agent rival;
    Service service{0,0,0,0,0,0};FarmFlowPlan flows;
    wait.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    std::array<uint8_t,8> shops;uint64_t random=seed^0xa37108e62d045fb9ULL;for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    {
        Sim original_sim(config);Source original;public_router::Agent other;
        original.reset(agent::runtime::make_agent_init(original_sim,0));other.reset(agent::runtime::make_agent_init(original_sim,1));
        for(int step=0;step<719;++step) {
            std::copy_n(shops.begin(),original_sim.st.n_shops,original_sim.st.shops);
            Action pair[2];original.act(agent::runtime::make_observation(original_sim,0),{},pair[0]);other.act(agent::runtime::make_observation(original_sim,1),{},pair[1]);
            const auto& farm=original_sim.st.farms[0];const int day=original_sim.st.day;
            if(day>=purchase_step/24)for(int u=0;u<pair[0].n_units;++u)if(farm.pos_y[u]*10+farm.pos_x[u]==animal_cell) {
                const uint32_t bit=uint32_t(1)<<day;
                if(pair[0].units[u].op==OP_FEED)service.feed|=bit;
                if(pair[0].units[u].op==OP_CARE)service.care|=bit;
                if(pair[0].units[u].op==OP_COLLECT_FERTILIZER)service.collect_fertilizer|=bit;
                if(pair[0].units[u].op==OP_HARVEST)service.harvest|=bit;
            }
            original_sim.step(pair[0],pair[1]);
        }
    }
    std::vector<RecordedDay> days;days.reserve(30);
    for(int step=0;step<719;++step) {
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);if(sim.st.hour==0)days.emplace_back(sim);
        Action pair[2];wait.act(agent::runtime::make_observation(sim,0),{},pair[0]);rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
        validate_action(pair[0],agent::runtime::make_observation(sim,0));const auto before=sim;
        const auto accepted=accepted_market(before,pair);
        for(const auto& slot:accepted.slots) {
            const auto& trade=slot.trades[0];
            if(trade.op==M_SELL)flows.sales[before.st.day][trade.item]+=trade.n;
            if(trade.op==M_BUY_PRODUCT)flows.buys[before.st.day][trade.item]+=trade.n;
        }
        sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
    }
    std::ofstream model(folder/"model.hpp");model<<"#pragma once\n#include \"../../include/animal_investment_value.hpp\"\nnamespace compositions::"<<folder.filename().string()<<" {inline FarmFlowPlan flows(){FarmFlowPlan p;\n";
    auto flow=[&](const char* name,const ProductFlows& data){model<<"p."<<name<<"={{";for(int day=0;day<30;++day){model<<(day?",":"")<<"{{";for(int item=0;item<N_PRODUCTS;++item)model<<(item?",":"")<<data[day][item];model<<"}}";}model<<"}};\n";};
    flow("sales",flows.sales);flow("buys",flows.buys);model<<"return p;}inline Service service(){return {0,"<<service.feed<<"u,"<<service.care<<"u,"<<service.collect_fertilizer<<"u,"<<service.harvest<<"u,0};}}\n";model.close();
    std::ofstream log(folder/"compiled.csv");log<<"day,item,extra_hires,solved,seconds,endpoint_equal,finance_equal\n";
    std::vector<AnimalEntry> entries;
    for(int day:entry_days)for(int item=GOOSE;item<=SHEEP;++item) {
        const auto& source=days[day];auto problem=source.problem;auto markets=source.own;
        const auto start=problem.start.managed_tiles[animal_cell].state;
        if(start.animal>=0 || start.kind==ManagedTileKind::LOCKED){std::cerr<<"reserved tile unavailable on day "<<day<<'\n';return 3;}
        // The original coop may be built during the purchase day. Its old
        // structure job belongs to this investment's invalidation closure.
        // Preserve a preceding crop harvest: this is a dated tile rotation.
        TileWork work;work.tile=animal_cell;bool builds=false;
        for(const auto& old:problem.tile_work)if(old.tile==animal_cell)for(auto action:old.actions) {
            if(action.op==OP_BUILD_COOP || action.op==OP_BUILD_PASTURE){action.op=item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE;builds=true;}
            work.actions.push_back(action);
        }
        std::erase_if(problem.tile_work,[&](const TileWork& w){return w.tile==animal_cell;});
        if(!builds && start.kind!=structure_for(item)) {
            if(start.kind==ManagedTileKind::CROP){std::cerr<<"no crop release contract on day "<<day<<'\n';return 3;}
            if(start.kind!=ManagedTileKind::EMPTY)work.actions.push_back({OP_DIG});
            work.actions.push_back({uint8_t(item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE)});
        }
        work.actions.push_back({OP_PLACE,int16_t(item)});work.actions.push_back({OP_FEED});work.actions.push_back({OP_CARE});problem.tile_work.push_back(work);
        auto& end=*problem.required_end_tiles[animal_cell].exact_state;end={};end.kind=structure_for(item);end.animal=item;end.age_days=1;end.pending_care_bonus=1;end.fertilizer_available=true;
        const auto path=folder/(folder.filename().string()+"_d"+std::to_string(day)+"_i"+std::to_string(item));fs::create_directories(path);
        auto order=[&](int op,int product,int quantity){return insert_funded_order(source,problem,markets,shops,op,product,quantity,
            day==purchase_step/24?purchase_step%24:-1,op==M_BUY_ANIMAL?purchase_order:-1);};
        if(!order(M_BUY_ANIMAL,item,1)){std::ofstream(path/"funding_failure.txt")<<"No insertion preserves existing accepted market obligations.\n";continue;}
        if(problem.end_shed[WHEAT]>0)--problem.end_shed[WHEAT];else if(!order(M_BUY_PRODUCT,WHEAT,1)){std::ofstream(path/"funding_failure.txt")<<"New feed purchase cannot preserve existing market obligations.\n";continue;}
        bool accepted=false;
        for(int extra=0;extra<=1 && !accepted;++extra) {
            if(extra){if(!order(M_HIRE,0,1)){std::ofstream(path/"extra_hire_funding_failure.txt")<<"Extra worker is not funded.\n";break;}++problem.worker_count;}
            day_scheduler::prepare_problem(problem);save_problem_json(problem,path/"problem.json");
            day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;const auto result=day_scheduler::solve(problem,options);
            bool equal=false,finance=false;
            if(result.schedule) {
                auto actions=*result.schedule;auto rebuilt=source.start;auto market_control=source.start;
                for(int hour=0;hour<24;++hour) {
                    std::copy_n(shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);std::copy_n(shops.begin(),market_control.st.n_shops,market_control.st.shops);
                    actions[hour].n_orders=markets[hour].n_orders;std::copy_n(markets[hour].orders,markets[hour].n_orders,actions[hour].orders);actions[hour].finalize();
                    rebuilt.step(actions[hour],source.rival[hour]);market_control.step(markets[hour],source.rival[hour]);
                }
                const auto& farm=rebuilt.st.farms[0];equal=farm.n_quadrants==days[day+1].start.st.farms[0].n_quadrants;
                std::ofstream differences(path/("endpoint_extra"+std::to_string(extra)+".txt"));
                if(!equal)differences<<"quadrants "<<farm.n_quadrants<<" required "<<days[day+1].start.st.farms[0].n_quadrants<<'\n';
                for(int i=0;i<N_ITEMS;++i){equal&=farm.shed[i]==problem.end_shed[i];if(farm.shed[i]!=problem.end_shed[i])differences<<"shed "<<i<<" actual "<<farm.shed[i]<<" required "<<problem.end_shed[i]<<'\n';}
                for(int i=0;i<N_CROPS;++i)equal&=farm.seeds[i]==problem.end_seeds[i];
                for(int cell=0;cell<100;++cell){auto actual=managed(farm.tiles[cell/10][cell%10],day+1);const auto desired=*problem.required_end_tiles[cell].exact_state;if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};equal&=actual==desired;if(actual!=desired){differences<<"tile "<<cell;for(const auto& t:{actual,desired})differences<<" [kind "<<int(t.kind)<<" animal "<<t.animal<<" age "<<t.age_days<<" yield "<<t.stored_units<<" dry "<<t.consecutive_dry_days<<" care "<<t.pending_care_bonus<<" fert "<<t.fertilizer_available<<']';differences<<'\n';}}
                save_actions(actions,path/("attempt_extra"+std::to_string(extra)+".txt"));
                finance=farm.money==market_control.st.farms[0].money && rebuilt.st.farms[1].money==market_control.st.farms[1].money;
                // The control preserves markets but does not place the new
                // animal. It checks funding/trades, not physical realization.
                if(equal && finance){AnimalEntry entry{item,guarded(source,problem,actions)};entries.push_back(entry);export_entry(path,entry);save_actions(actions,path/"schedule.txt");accepted=true;}
            }
            log<<day<<','<<item<<','<<extra<<','<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<','<<finance<<'\n';log.flush();
            std::cout<<"day="<<day<<" item="<<item<<" extra="<<extra<<" solved="<<bool(result.schedule)<<" endpoint="<<equal<<" finance="<<finance<<std::endl;
        }
    }
    std::ofstream(folder/"summary.json")<<"{\"seed\":"<<seed<<",\"compiled_entries\":"<<entries.size()<<",\"wait_cash\":"<<sim.st.farms[0].money<<",\"source\":\""<<source_name<<"\",\"purchase_step\":"<<purchase_step<<",\"purchase_order\":"<<purchase_order<<",\"cell\":"<<animal_cell<<",\"original\":"<<original<<"}\n";
}
