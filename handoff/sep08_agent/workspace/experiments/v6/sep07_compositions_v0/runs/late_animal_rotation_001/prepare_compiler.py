from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
season_path = RUN / 'source/season.hpp'
season = season_path.read_text()
season = season.replace('std::vector<RecordedDay> days;', 'std::vector<RecordedDay> days;\n    std::array<Farm,30> end_farms;')
season = season.replace('const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);', '''const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
            end_farms[before.st.day]=sim.st.farms[0];''')
season = season.replace('        std::array<int,100> active;', '''        // Pad only the offline day contract with a PASS hour. Actual games
        // still end after hour 22 on day 29; no padded output is scored.
        sim.st.done=false; sim.cfg.episode_steps=744;
        Action padding[2];
        for(int p=0;p<2;++p){padding[p].n_units=sim.st.farms[p].n_units;padding[p].finalize();}
        const auto before_padding=sim;
        sim.step(padding[0],padding[1]);
        append_contract(days.back(),before_padding,sim,padding);
        end_farms[29]=sim.st.farms[0];
        std::array<int,100> active;''')
season_path.write_text(season)
original = (EXP / 'src/compile_crop_rotation.cpp').read_text()
utilities = original[original.index('std::array<int,N_ITEMS> flow'):original.index('int main(')]
utilities = utilities.replace('if(job.op==OP_FEED)--result[WHEAT];', 'if(job.op==OP_FEED)--result[WHEAT];\n        if(job.op==OP_PLACE && is_animal(job.arg))--result[job.arg];')
main = original[original.index('int main('):]
first = main.index('    Config config;')
main = '''int main(int argc,char** argv) {
    if(argc!=7)return 2;
    const fs::path directory=fs::absolute(argv[1]);
    const uint64_t seed=std::stoull(argv[2]);
    Rotation rotation;rotation.cell=std::stoi(argv[3]);rotation.first=std::stoi(argv[4]);rotation.item=std::stoi(argv[5]);
    const double seconds=std::stod(argv[6]);
    if(fs::exists(directory)||rotation.cell<0||rotation.cell>=100||rotation.first<13||rotation.first>22||!is_animal(rotation.item)||seconds<=0)return 2;
    const std::vector<Rotation> rotations={rotation}; const int first=rotation.first;
    const bool cancel_unused_seeds=true,next_day_tomato_sales=false;
    fs::create_directories(directory/"days");const auto name=directory.filename().string();
    const Season season(seed);
''' + main[first:]
main = main.replace('day<29;', 'day<30;')
main = main.replace('season.days[day+1].start.st.farms[0].discarded[i]', 'season.end_farms[day].discarded[i]')
main = main.replace('if(delta<0 && (i!=WHEAT && i!=FERTILIZER))', 'if(delta<0 && (i!=WHEAT && i!=FERTILIZER && !is_animal(i)))')
main = main.replace('M_BUY_PRODUCT,i,-delta', 'is_animal(i)?M_BUY_ANIMAL:M_BUY_PRODUCT,i,-delta')
main = main.replace('if(!kept){std::ofstream', 'if(!kept){std::ofstream')
main = main.replace('            market_contract(trial,orders);', '''            if(day==29) {
                // Every sale must occur before the real last action, hour 22.
                for(int s=0;s<orders[23].n_orders;++s) {
                    const auto order=orders[23].orders[s];
                    if(order.op!=M_NONE && !add_order(orders,22,22,order.op,order.item,order.n))return 3;
                }
                orders[23].n_orders=0;orders[23].finalize();
            }
            market_contract(trial,orders);''')
main = main.replace('for(int h=0;h<24;++h)step(&actions[h]);\n                equal=endpoint_equal(sim.st.farms[0],trial,day+1);', '''for(int h=0;h<24 && !sim.st.done;++h)step(&actions[h]);
                auto endpoint=sim;
                if(day==29) {
                    endpoint.st.done=false;endpoint.cfg.episode_steps=744;
                    Action padding[2];for(int p=0;p<2;++p){padding[p].n_units=endpoint.st.farms[p].n_units;padding[p].finalize();}
                    endpoint.step(padding[0],padding[1]);
                }
                equal=endpoint_equal(endpoint.st.farms[0],trial,day+1);''')
main = main.replace('GuardedSequenceAgent<Source>(sequence)', 'LateSequence<Source>(sequence)')
header = '''#include "season.hpp"
#include "sequence.hpp"

struct Rotation { int cell=22, first=17, item=GOOSE; };

std::vector<TileWorkAction> source_work(const DayProblem& problem,int cell) {
    for(const auto& work:problem.tile_work)if(work.tile==cell)return work.actions;
    return {};
}

std::vector<TileWorkAction> rotation_work(const Rotation& r,int day,const Tile& tile,
        const std::vector<TileWorkAction>& original) {
    std::vector<TileWorkAction> result;
    auto add=[&](int op,int arg=-1){TileWorkAction job;job.op=op;job.arg=arg;result.push_back(job);};
    if(day==r.first) {
        bool released=false;
        for(const auto& job:original) {
            if(job.op==OP_PLANT)break;
            result.push_back(job);released|=job.op==OP_HARVEST;
        }
        if(!released || tile.kind!=T_PLANT || CROPS[tile.what].ongoing)std::abort();
        add(r.item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE);
        add(OP_PLACE,r.item);
    }
    if(day<29)add(OP_FEED);
    const auto& species=ANIMALS[r.item-GOOSE];
    int last=r.first+species.first_yield_day;
    while(last+species.interval<=29)last+=species.interval;
    if(day<last-1)add(OP_CARE);
    if(day>r.first) {
        if(tile.fertilizer_available)add(OP_COLLECT_FERTILIZER);
        if(tile.yield_units>0)add(OP_HARVEST,species.product);
    }
    return result;
}

Tile tile_endpoint(const Sim& start,int cell,std::vector<TileWorkAction>& work) {
    auto sim=start;sim.cfg.episode_steps=744;
    auto& farm=sim.st.farms[0];farm.n_units=1;
    farm.pos_x[0]=cell%10;farm.pos_y[0]=cell/10;
    for(int i=0;i<N_ITEMS;++i)farm.inv_add(0,i,20);
    std::fill_n(farm.seeds,N_CROPS,20);
    Action pass;pass.n_units=1;pass.finalize();
    for(auto& job:work) {
        const auto before=farm.produced[job.arg>=0?job.arg:0];
        Action action=pass;action.units[0]={job.op,uint8_t(std::max(0,int(job.arg))),1};action.finalize();
        sim.step(action,pass);
        if(job.op==OP_HARVEST) {
            job.output_item=job.arg;job.output_quantity=farm.produced[job.arg]-before;
            if(job.output_quantity<=0)std::abort();
        }
        if(job.op==OP_COLLECT_FERTILIZER){job.output_item=FERTILIZER;job.output_quantity=1;}
    }
    const int day=sim.st.day;
    while(sim.st.day==day && !sim.st.done)sim.step(pass,pass);
    return farm.tiles[cell/10][cell%10];
}

'''
(RUN / 'source/compile.cpp').write_text(header + utilities + main)
sequence = (EXP / 'include/guarded_sequence.hpp').read_text()
sequence = sequence.replace('#include "guarded_day.hpp"', '#include "experiments/v6/sep07_compositions_v0/include/guarded_day.hpp"')
sequence = sequence.replace('GuardedSequenceAgent', 'LateSequence').replace('plan.day>=29', 'plan.day>=30')
(RUN / 'source/sequence.hpp').write_text(sequence)
cmake = (RUN / 'CMakeLists.txt').read_text()
cmake += '''
foreach(leaf off on)
    add_executable(compile_${leaf} source/compile.cpp
        "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
        "${EXPERIMENT}/league/public_router/source/agent.cpp")
    target_include_directories(compile_${leaf} BEFORE PRIVATE "${REPOSITORY}")
    target_compile_options(compile_${leaf} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(compile_${leaf} PRIVATE DaySolver::scheduler)
endforeach()
target_compile_definitions(compile_on PRIVATE LATE_BERRY=1)
'''
(RUN / 'CMakeLists.txt').write_text(cmake)
