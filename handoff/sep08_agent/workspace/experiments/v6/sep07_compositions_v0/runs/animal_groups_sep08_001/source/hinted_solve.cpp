#include "season.hpp"
#include "day_solver/src/components/native_light_exact/exact.hpp"

int main(int argc,char** argv){
    if(argc!=8 || fs::exists(argv[7]))return 2;
    const auto problem=load_problem_json(argv[1]);
    const uint64_t seed=std::stoull(argv[2]);const int day=std::stoi(argv[3]);
    const double seconds=std::stod(argv[4]);const int workers=std::stoi(argv[5]),mode=std::stoi(argv[6]);
    if(day<0 || day>=30 || seconds<=0 || workers<=0 || mode<0 || mode>1)return 2;
    const fs::path out(argv[7]);fs::create_directories(out);
    const Season season(seed);const auto& original=season.days[day];
    std::array<std::vector<int>,100> tasks;
    std::vector<TileWorkAction> actions;
    std::array<bool,100> unchanged{};
    for(const auto& work:problem.tile_work){
        for(const auto& action:work.actions){tasks[work.tile].push_back(actions.size());actions.push_back(action);}
        for(const auto& old:original.problem.tile_work)if(old.tile==work.tile && old.actions.size()==work.actions.size()){
            bool equal=true;
            for(size_t i=0;i<work.actions.size();++i){
                const auto& a=work.actions[i];const auto& b=old.actions[i];
                equal&=a.op==b.op && a.arg==b.arg && a.quantity==b.quantity && a.output_item==b.output_item && a.output_quantity==b.output_quantity;
            }
            unchanged[work.tile]=equal;
        }
    }
    day_native::InternalHint hint;
    std::array<int,100> used{};auto sim=original.start;
    std::ofstream assigned(out/"ASSIGNMENTS.csv");assigned<<"task,cell,worker,hour,operation\n";
    for(int h=0;h<24 && !sim.st.done;++h){
        const auto& farm=sim.st.farms[0];
        for(int u=0;u<farm.n_units;++u){
            const auto& action=original.own[h].units[u];const int cell=farm.pos_y[u]*10+farm.pos_x[u];
            if(!unchanged[cell] || used[cell]>=int(tasks[cell].size()))continue;
            const int id=tasks[cell][used[cell]];const auto& expected=actions[id];
            if(action.op!=expected.op || ((action.op==OP_PLANT || action.op==OP_PLACE) && action.arg!=expected.arg))continue;
            hint.assignments.push_back({id,u,h,{},{}});++used[cell];
            assigned<<id<<','<<cell<<','<<u<<','<<h<<','<<+action.op<<'\n';
        }
        sim.step(original.own[h],original.rival[h]);
    }
    for(int cell=0;cell<100;++cell)if(unchanged[cell] && used[cell]!=int(tasks[cell].size()))std::abort();
    day_native::light::HintOptions hints;
    if(mode)hints.documents[day_native::HintKind::partial]=hint;
    day_native::light::SolveOptions options;options.seconds=seconds;options.workers=workers;
    const auto result=day_native::light::solve(problem,hints,options,2);
    bool requirements=false,invariants=false,clean=false;
    if(result.schedule){
        const auto replay=replay_schedule(problem,*result.schedule);
        requirements=replay.requirements_satisfied;invariants=replay.invariants_satisfied;clean=replay.errors.empty();
        save_actions(*result.schedule,out/"actions.txt");
        std::ofstream errors(out/"errors.txt");for(const auto& error:replay.errors)errors<<error<<'\n';
    }
    std::ofstream(out/"STATUS.json")<<"{\"mode\":"<<mode<<",\"hinted_tasks\":"<<(mode?hint.assignments.size():0)
        <<",\"tasks\":"<<actions.size()<<",\"solver_status\":"<<int(result.status)<<",\"solved\":"<<result.solved
        <<",\"schedule\":"<<bool(result.schedule)<<",\"build_seconds\":"<<result.build_seconds
        <<",\"solver_seconds\":"<<result.solver_seconds<<",\"branches\":"<<result.branches
        <<",\"requirements\":"<<requirements<<",\"invariants\":"<<invariants<<",\"clean\":"<<clean<<"}\n";
    std::cout<<"mode="<<mode<<" tasks="<<actions.size()<<" hinted="<<(mode?hint.assignments.size():0)
        <<" solved="<<result.solved<<" seconds="<<result.solver_seconds<<std::endl;
}
