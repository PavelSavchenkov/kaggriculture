#include "case.hpp"
#include "placement.hpp"
#include "day_jobs.hpp"
#include "verify.hpp"
#include "trace/case.hpp"
#include "trace/field_effect.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
using namespace kag;
using namespace kag::agents::day_policy_contract;
using namespace schedule_search;
using contract_benchmark::Case;

// Grid-only chronological counterfactual. Preserve each recorded generation's
// service outcome, but choose its site once with place_day and never move it.
// Worker schedules are tested separately; an excluded prefix day is not passed
// off as a successful execution. Native night updates run on the changed grid.
bool product(const Tile& t) { return t.kind==T_PLANT || t.has_animal; }
void masks(Farm& f) {
    for(auto* m:{f.empty_mask,f.plant_mask,f.animal_mask,f.decay_mask})m[0]=m[1]=0;
    f.next_decay_step=INT_MAX; // The recorded last-hour field state already decayed.
    for(int c=0;c<100;++c){const auto& t=f.tiles[c/10][c%10];const auto bit=uint64_t{1}<<(c%64);
        if(t.kind==T_EMPTY)f.empty_mask[c/64]|=bit;
        if(t.kind==T_PLANT)f.plant_mask[c/64]|=bit;
        if(t.has_animal)f.animal_mask[c/64]|=bit;
    }
}
int main(int argc,char** argv) {
    if(argc<5 || argc>8)throw std::runtime_error("progression cohort.txt original.bin output.bin audit.csv [original_placement_control] [animal_reserve] [placement_style]");
    const bool control=argc>5 && std::stoi(argv[5]);
    const int reserve=argc>6?std::stoi(argv[6]):0;
    const auto style=argc>7?static_cast<PlacementStyle>(std::stoi(argv[7])):PlacementStyle::Legacy;
    std::ifstream file(argv[2],std::ios::binary);Case row;
    std::map<std::tuple<int,int,int>,Case> cases;
    while(file.read(reinterpret_cast<char*>(&row),sizeof(row)))cases[{row.game,row.seat,row.day}]=row;
    std::ifstream cohort(argv[1]);std::ofstream out(argv[3],std::ios::binary),audit(argv[4]);
    audit<<"game,seat,rank,day,original_reason,progression_reason,moved_products,standalone_housing\n";
    int game_id,seat,rank;std::string path;int games=0;
    while(cohort>>game_id>>seat>>rank>>path) {
        auto game=load_case(path);auto states=validate_case(game);
        Tile shadow[100];std::copy_n(&states[0].st.farms[seat].tiles[0][0],100,shadow);
        int identity[100];std::iota(identity,identity+100,0);
        bool blocked=false;
        for(int day=0;day<30;++day) {
            auto c=cases.at({game_id,seat,day});const auto original=c;
            if(control)for(int cell=0;cell<100;++cell) {
                const auto& a=shadow[cell];const auto& b=states[day*24].st.farms[seat].tiles[cell/10][cell%10];
                if(a.kind!=b.kind || a.what!=b.what || a.has_animal!=b.has_animal || a.watered_today!=b.watered_today ||
                    a.fed_today!=b.fed_today || a.cared_today!=b.cared_today || a.fertilizer_available!=b.fertilizer_available ||
                    a.consecutive_dry!=b.consecutive_dry || a.yield_units!=b.yield_units || a.pending_care_bonus!=b.pending_care_bonus ||
                    a.planted_day!=b.planted_day || a.fertilized_until_day!=b.fertilized_until_day || a.max_lifespan_step!=b.max_lifespan_step) {
                    std::cerr<<game_id<<" seat="<<seat<<" day="<<day<<" cell="<<cell<<" actual_kind="<<int(a.kind)<<" expected_kind="<<int(b.kind)<<'\n';
                    throw std::runtime_error("original placement progression parity");
                }
            }
            std::string reason="ok";int moved=0,housing_count=0;
            auto fail=[&](const char* s){if(reason=="ok")reason=s;};
            if(blocked)fail("blocked_prefix");
            std::fill_n(c.input.events,100,0);bool claimed[100]{};
            for(int cell=0;cell<100;++cell) {
                auto t=shadow[cell];t.planted_day-=day;t.fertilized_until_day-=day;
                t.max_lifespan_step=t.max_lifespan_step<0?INT_MAX:t.max_lifespan_step-day*24;c.input.grid[cell]=t;
            }
            for(int cell=0;cell<100;++cell)if(product(original.input.grid[cell])) {
                const int target=identity[cell];const auto& wanted=original.input.grid[cell];
                if(target<0 || !product(shadow[target]) || shadow[target].kind!=wanted.kind || shadow[target].what!=wanted.what){fail("missing_product_identity");continue;}
                claimed[target]=true;c.input.events[target]=original.input.events[cell];moved+=target!=cell;
            }
            // Non-product clearing jobs follow the corresponding surviving weed
            // or housing where possible. Never remove an event to gain success.
            for(int cell=0;cell<100;++cell)if(!product(original.input.grid[cell]) && original.input.events[cell]) {
                int target=-1,best=INT_MAX;
                for(int at=0;at<100;++at)if(!claimed[at] && !product(shadow[at]) && shadow[at].kind==original.input.grid[cell].kind) {
                    const int cost=distance(at,cell);if(cost<best)best=cost,target=at;
                }
                if(target<0){fail("missing_clear_target");continue;}
                claimed[target]=true;c.input.events[target]=original.input.events[cell];
            }
            if(reason=="ok" && !std::strcmp(original.reason,"eligible") && !detail::valid_input(c.input))fail("invalid_mapped_input");
            if(reason!="ok")std::snprintf(c.reason,sizeof(c.reason),"progression_%s",reason.c_str());
            out.write(reinterpret_cast<const char*>(&c),sizeof(c));
            if(day==29 || blocked){audit<<game_id<<','<<seat<<','<<rank<<','<<day<<','<<original.reason<<','<<reason<<','<<moved<<",0\n";continue;}

            // Recover creation order and identities from successful operations.
            int new_cell[100],latest[100],built[100]{};std::fill_n(latest,100,-1);int count=0;
            for(int h=0;h<24;++h) {
                const int step=day*24+h;const auto& sim=states[step];const auto& f=sim.st.farms[seat];
                auto accepted=sim.sanitize_joint_actions(game.turns[step].actions[0],game.turns[step].actions[1]);
                Tile tiles[100];std::copy_n(&f.tiles[0][0],100,tiles);
                for(int u=0;u<f.n_units;++u) {
                    const auto a=accepted[seat].units[u];const int cell=f.pos_y[u]*10+f.pos_x[u];
                    const bool establish=a.op==OP_PLANT || (a.op==OP_PLACE && is_animal(a.arg) && !tiles[cell].has_animal &&
                        tiles[cell].kind==(ANIMALS[a.arg-GOOSE].structure==ST_COOP?T_COOP:T_PASTURE));
                    if(establish){if(count==100)throw std::runtime_error("creation overflow");new_cell[count]=cell;latest[cell]=count++;}
                    if(a.op==OP_BUILD_COOP || a.op==OP_BUILD_PASTURE)built[cell]=1;
                    field_effect(tiles[cell],a);
                }
            }
            if(count!=original.input.establish_count)throw std::runtime_error("creation extraction mismatch");
            auto plan=compile_day_jobs(c.input,11,0,false);
            const int first_new=plan.count-count;
            if(first_new<0 || (count && !plan.jobs[first_new].new_site))throw std::runtime_error("compiled establishment mapping");
            auto placement_state=detail::initial_state(c.input);
            place_day(agent::runtime::make_observation(placement_state,0),plan,reserve,true,style);
            if(control)for(int n=0;n<count;++n)plan.jobs[first_new+n].tile=new_cell[n];
            for(int n=0;n<count;++n)if(plan.jobs[first_new+n].tile<0)fail("no_placement_site");
            if(reason=="missing_product_identity" || reason=="no_placement_site")blocked=true;
            if(blocked){audit<<game_id<<','<<seat<<','<<rank<<','<<day<<','<<original.reason<<','<<reason<<','<<moved<<",0\n";continue;}

            // Replay final-hour field work without night, then transplant each
            // generation's resulting attributes to its permanent chosen site.
            auto night=states[day*24+23];night.st.hour=22;
            night.step(game.turns[day*24+23].actions[0],game.turns[day*24+23].actions[1]);
            const auto recorded=night.st.farms[seat];
            const int quadrants=recorded.n_quadrants;
            for(int cell=0;cell<100;++cell) {
                if(shadow[cell].kind==T_LOCKED && quadrant_of(cell%10,cell/10,10)<quadrants)shadow[cell]=Tile{};
                if(c.input.events[cell]&Clear)shadow[cell]=Tile{};
            }
            int next_identity[100];std::fill_n(next_identity,100,-1);
            for(int cell=0;cell<100;++cell)if(product(original.input.grid[cell]) && identity[cell]>=0) {
                const int target=identity[cell];
                if(latest[cell]>=0)shadow[target]=Tile{};
                else {shadow[target]=recorded.tiles[cell/10][cell%10];if(product(shadow[target]))next_identity[cell]=target;}
            }
            for(int n=0;n<count;++n) {
                const int cell=new_cell[n],target=plan.jobs[first_new+n].tile;
                // Excluded prefix days can plant and clear a new generation
                // before replacing it. It has no surviving grid identity. Keep
                // the original exclusion, but do not block every later day.
                if(latest[cell]!=n)continue;
                if(product(shadow[target]))throw std::runtime_error("placement moved an existing product");
                shadow[target]=recorded.tiles[cell/10][cell%10];next_identity[cell]=target;
            }
            // Housing-only prefix actions are outside the contract. Retain them
            // in this geometry diagnostic using the same housing preference.
            for(int cell=0;cell<100;++cell)if(built[cell] && latest[cell]<0 && !recorded.tiles[cell/10][cell%10].has_animal &&
                (recorded.tiles[cell/10][cell%10].kind==T_COOP || recorded.tiles[cell/10][cell%10].kind==T_PASTURE)) {
                DayInput layout=c.input;std::copy_n(shadow,100,layout.grid);
                auto sim=detail::initial_state(layout);DayPlan p;p.relocate_new=true;p.count=1;p.jobs[0].new_site=true;p.jobs[0].count=1;
                p.jobs[0].steps[0]={OP_PLACE,uint8_t(recorded.tiles[cell/10][cell%10].kind==T_COOP?GOOSE:COW),1};
                place_day(agent::runtime::make_observation(sim,0),p,reserve,true,style);
                if(control)p.jobs[0].tile=cell;
                if(p.jobs[0].tile<0){fail("no_housing_site");blocked=true;break;}
                shadow[p.jobs[0].tile]=recorded.tiles[cell/10][cell%10];++housing_count;
            }
            auto& farm=night.st.farms[seat];std::copy_n(shadow,100,&farm.tiles[0][0]);masks(farm);
            if(control)for(int cell=0;cell<100;++cell)if(shadow[cell].kind!=recorded.tiles[cell/10][cell%10].kind) {
                std::cerr<<"pre-night mismatch game="<<game_id<<" day="<<day<<" tile="<<cell<<" shadow="<<int(shadow[cell].kind)<<" recorded="<<int(recorded.tiles[cell/10][cell%10].kind)<<" new="<<count<<'\n';
                for(int n=0;n<count;++n)std::cerr<<"establish "<<n<<" original="<<new_cell[n]<<" mapped="<<plan.jobs[first_new+n].tile<<'\n';
                throw std::runtime_error("pre-night original placement parity");
            }
            // Both farms already received last-hour decay; suppress its repeat.
            night.st.farms[1-seat].next_decay_step=INT_MAX;
            night.st.hour=23;night.st.day=day;night.st.step=day*24+23;
            night.step(Action{},Action{});
            std::copy_n(&night.st.farms[seat].tiles[0][0],100,shadow);
            std::copy_n(next_identity,100,identity);
            audit<<game_id<<','<<seat<<','<<rank<<','<<day<<','<<original.reason<<','<<reason<<','<<moved<<','<<housing_count<<'\n';
        }
        std::cout<<++games<<" game="<<game_id<<" seat="<<seat<<'\n'<<std::flush;
    }
}
