#pragma once
// Reuse the parent task witness; leave changed tile tasks free. Failure only
// rejects this restriction. The unrestricted day solver remains available.
#include "day_solver/src/components/native_light_exact/exact.hpp"
inline day_scheduler::Result fixed_repair(const RecordedDay& original,const DayProblem& problem,double seconds){
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
    for(int h=0;h<24 && !sim.st.done;++h){
        const auto& farm=sim.st.farms[0];
        for(int u=0;u<farm.n_units;++u){
            const auto& action=original.own[h].units[u];const int cell=farm.pos_y[u]*10+farm.pos_x[u];
            if(!unchanged[cell] || used[cell]>=int(tasks[cell].size()))continue;
            const int id=tasks[cell][used[cell]];const auto& expected=actions[id];
            if(action.op!=expected.op || ((action.op==OP_PLANT || action.op==OP_PLACE) && action.arg!=expected.arg))continue;
            hint.assignments.push_back({id,u,h,{},{}});++used[cell];
        }
        sim.step(original.own[h],original.rival[h]);
    }
    for(int cell=0;cell<100;++cell)if(unchanged[cell] && used[cell]!=int(tasks[cell].size()))std::abort();
    day_native::light::HintOptions hints;
    hints.documents[day_native::HintKind::fixed_partial]=hint;
    day_native::light::SolveOptions options;options.seconds=std::min(1.0,seconds);options.workers=4;
    const auto result=day_native::light::solve(problem,hints,options,2);
    day_scheduler::Result answer;answer.seconds=result.build_seconds+result.solver_seconds;
    if(result.schedule){
        const auto replay=replay_schedule(problem,*result.schedule);
        if(replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty())answer.schedule=result.schedule;
    }
    return answer;
}
