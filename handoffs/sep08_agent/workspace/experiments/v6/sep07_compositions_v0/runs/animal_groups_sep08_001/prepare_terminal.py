"""Carry lost output across days, and omit the new animal's final fertilizer pickup."""
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
old=RUN/'source/compile_v4.cpp';new=RUN/'source/compile_v5.cpp'
assert not new.exists()
source=old.read_text()
source=source.replace('if(tile.fertilizer_available)add(OP_COLLECT_FERTILIZER);',
    'if(day<29 && tile.fertilizer_available)add(OP_COLLECT_FERTILIZER);')
before='if(delta<0 && (i!=WHEAT && i!=FERTILIZER && !is_animal(i))) {std::cerr<<"unbuyable deficit day "<<day<<" item "<<i<<\'\\n\';return 3;}'
assert source.count(before)==1
source=source.replace(before,'''if(delta<0 && (i!=WHEAT && i!=FERTILIZER && !is_animal(i))){
                const int reduce=std::min<int64_t>(-delta,problem.end_shed[i]);
                problem.end_shed[i]-=reduce;delta+=reduce;
                if(delta<0){std::cerr<<"unbuyable deficit day "<<day<<" item "<<i<<'\\n';return 3;}
            }''')
source=source.replace('if(argc!=5)return 2;','if(argc!=5 && argc!=6)return 2;\n    const fs::path resume=argc==6?fs::absolute(argv[5]):fs::path{};')
point='    const bool cancel_unused_seeds=true,next_day_tomato_sales=false;'
source=source.replace(point,'''    auto read_actions=[](const fs::path& path){
        std::ifstream input(path);if(!input)std::abort();std::array<Action,24> result;
        for(auto& a:result){
            input>>a.n_units>>a.n_orders;
            for(int u=0;u<a.n_units;++u){int op,arg;input>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
            for(int s=0;s<a.n_orders;++s){int op,item;input>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
            a.finalize();
        }
        if(!input)std::abort();return result;
    };
''' + point)
point='        auto problem=source.problem;problem.start=actual.problem.start;'
assert source.count(point)==1
source=source.replace(point,'''        if(!resume.empty() && day<29 && fs::is_regular_file(resume/"days"/std::to_string(day)/"actions.txt")){
            const auto old=resume/"days"/std::to_string(day);
            const auto actions=read_actions(old/"actions.txt");
            const auto problem=load_problem_json(old/"problem.json");
            for(int h=0;h<24 && !sim.st.done;++h)step(&actions[h]);
            if(!endpoint_equal(sim.st.farms[0],problem,day+1))std::abort();
            const auto guard=guarded(actual,problem,actions);sequence.push_back(guard);
            const auto folder=directory/"days"/std::to_string(day);
            export_day(guard,name+"_d"+std::to_string(day),folder);save_problem_json(problem,folder/"problem.json");
            log<<day<<','<<problem.worker_count-source.problem.worker_count<<",1,1,0\\n";log.flush();
            std::cout<<"day="<<day<<" exact_prefix_reused=1"<<std::endl;continue;
        }
''' + point)
new.write_text(source)
cmake=RUN/'CMakeLists.txt';text=cmake.read_text()
assert text.count('estimate_v2 compile_v4)')==1
cmake.write_text(text.replace('estimate_v2 compile_v4)','estimate_v2 compile_v4 compile_v5)'))
(RUN/'TERMINAL_COMPILER.json').write_text(json.dumps({'original_sha256':hashlib.sha256(old.read_bytes()).hexdigest(),
    'new_sha256':hashlib.sha256(new.read_bytes()).hexdigest(),
    'changes':['When lost non-input output exceeds same-day sales, reduce its terminal inventory and carry the deficit forward to future sales.',
        'Omit final-day fertilizer collection on changed animal tiles; retain useful product harvest.',
        'Optional reuse of earlier certified complete days, each revalidated in the full live-opponent engine.'],
    'origins':['Continuation inventory conservation from the failed day27 carrot target.',
        'Prior late_animal_schedule_001 final-day fertilizer omission; no new external component.'],
    'scope':'Compiler repair and bounded service alternative. Full-season and deployable-policy validation remain required.'},indent=2)+'\n')
