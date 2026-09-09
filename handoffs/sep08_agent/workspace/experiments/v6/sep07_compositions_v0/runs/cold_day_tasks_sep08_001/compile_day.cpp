#include "../../include/day_contract.hpp"
#include "../../include/estimate.hpp"
#include "../compiler_labor_sep08_001/cold_farm.hpp"

using namespace compositions;
using namespace compositions::day_contract;
namespace fs=std::filesystem;

std::vector<Life> placed_farm() {
    auto lives=compiler_labor_data::cold_farm(2,2,0,7,12,8);
    std::array<int,100> release{},previous{};previous.fill(-1);
    int allocated=1;
    for(auto& life:lives) {
        bool available=false;
        for(int cell=0;cell<100;++cell)
            available|=release[cell]<=life.start && quadrant_of(cell%10,cell/10,10)<allocated;
        int best=-1;double score=1e100;
        for(int cell=0;cell<100;++cell)if(release[cell]<=life.start) {
            const int x=cell%10,y=cell/10,q=quadrant_of(x,y,10);
            if(available && q>=allocated)continue;
            const double value=5*q+compositions::shed_distance(x,y)*(is_animal(life.item)?2.0:1.0)
                -(is_animal(life.item)&&is_animal(previous[cell])?1.5:0.0)+cell*0.0001;
            if(value<score){score=value;best=cell;}
        }
        if(best<0)std::abort();
        allocated=std::max(allocated,quadrant_of(best%10,best/10,10)+1);
        life.x=best%10;life.y=best/10;release[best]=life.end;previous[best]=life.item;
    }
    return lives;
}

DayProblem make_problem(std::span<const Life> lives,int service_mode) {
    Sim initial(Config{});DayProblem p;p.worker_count=9;
    for(int cell=0;cell<100;++cell) {
        const auto state=managed(initial.st.farms[0].tiles[cell/10][cell%10],0);
        p.start.managed_tiles.push_back({int8_t(cell%10),int8_t(cell/10),state});
        EndTileRequirement end;end.tile=cell;end.exact_state=state;p.required_end_tiles.push_back(end);
    }
    int seed_counts[N_CROPS]{},animal_counts[N_ANIMALS]{},wheat=0;
    for(const auto& life:lives)if(life.start/24==0) {
        const int cell=life.y*10+life.x;
        if(quadrant_of(life.x,life.y,10)!=0)std::abort();
        TileWork work;work.tile=cell;
        auto add=[&](int op,int arg=-1){TileWorkAction a;a.op=op;a.arg=arg;work.actions.push_back(a);};
        ManagedTileState end;end.age_days=1;
        if(is_animal(life.item)) {
            ++animal_counts[life.item-GOOSE];
            const bool serve=service_mode==0 || (service_mode==1 && life.item!=COW);
            add(life.item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE);add(OP_PLACE,life.item);
            if(serve){add(OP_FEED);add(OP_CARE);++wheat;}
            end.kind=structure_for(life.item);end.animal=life.item;end.fertilizer_available=true;
            end.consecutive_dry_days=serve?0:1;end.pending_care_bonus=serve?1:0;
        } else {
            ++seed_counts[life.item];add(OP_PLANT,life.item);add(OP_WATER);
            end.kind=ManagedTileKind::CROP;end.crop=life.item;end.stored_units=CROPS[life.item].ongoing?0:1;
        }
        p.tile_work.push_back(std::move(work));p.required_end_tiles[cell].exact_state=end;
    }
    int slot=0;
    auto buy=[&](int op,int item,int quantity){if(quantity)p.market_plan.push_back({0,int8_t(slot++),uint8_t(op),int16_t(item),quantity,0});};
    for(int i=0;i<N_CROPS;++i)buy(M_BUY_SEED,i,seed_counts[i]);
    for(int i=0;i<N_ANIMALS;++i)buy(M_BUY_ANIMAL,GOOSE+i,animal_counts[i]);
    buy(M_BUY_PRODUCT,WHEAT,wheat);
    int hour=0;
    for(int i=0;i<8;++i) {
        if(slot==10){++hour;slot=0;}
        p.market_plan.push_back({int8_t(hour),int8_t(slot++),M_HIRE,-1,1,0});
    }
    day_scheduler::prepare_problem(p);
    return p;
}

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    const fs::path out=argv[1];const double seconds=std::stod(argv[2]);
    if(fs::exists(out) || seconds<=0)return 2;
    fs::create_directories(out);
    const auto lives=placed_farm();
    std::ofstream plan(out/"plan.inc");plan<<"inline const std::vector<compositions::Life> lives={\n";
    for(const auto& l:lives)plan<<'{'<<l.item<<','<<l.start<<','<<l.end<<','<<l.x<<','<<l.y<<"},\n";
    plan<<"};\n";
    std::ofstream rows(out/"RESULTS.json");rows<<'[';
    for(int mode=0;mode<3;++mode) {
        if(mode)rows<<',';
        const auto folder=out/("mode"+std::to_string(mode));fs::create_directories(folder);
        const auto p=make_problem(lives,mode);save_problem_json(p,folder/"problem.json");
        day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
        const auto result=day_scheduler::solve(p,options);
        bool exact=false;double cash=0;int faults=0;
        if(result.schedule) {
            save_actions(*result.schedule,folder/"schedule.txt");
            Sim sim(Config{});
            for(int h=0;h<24;++h) {
                const auto& a=(*result.schedule)[h];Action pass;pass.n_units=1;pass.finalize();
                validate_action(a,agent::runtime::make_observation(sim,0));
                const auto check=sim.diagnose_joint_actions(a,pass);
                faults+=check.players[0].requested_unit_actions-check.players[0].successful_unit_actions;
                sim.step(a,pass);
            }
            exact=true;const auto& f=sim.st.farms[0];cash=f.money;
            for(int cell=0;cell<100;++cell) {
                auto actual=managed(f.tiles[cell/10][cell%10],1);const auto want=*p.required_end_tiles[cell].exact_state;
                if(want.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                exact&=actual==want;
            }
            for(int i=0;i<N_ITEMS;++i)exact&=f.shed[i]==p.end_shed[i] && f.discarded[i]==0;
            for(int i=0;i<N_CROPS;++i)exact&=f.seeds[i]==p.end_seeds[i];
            exact&=faults==0;
        }
        rows<<"{\"mode\":"<<mode<<",\"solved\":"<<(result.schedule?"true":"false")<<",\"full_engine_endpoint_equal\":"<<(exact?"true":"false")
            <<",\"seconds\":"<<result.seconds<<",\"cash_after_day0\":"<<cash<<",\"unit_faults\":"<<faults<<'}';rows.flush();
        std::cout<<"mode="<<mode<<" solved="<<bool(result.schedule)<<" exact="<<exact<<" cash="<<cash<<" seconds="<<result.seconds<<std::endl;
    }
    rows<<"]\n";
}
