"""Preserve v1 and add concrete certificates and failure traces in a new compiler."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
old = RUN/'source/compile.cpp'
new = RUN/'source/compile_v2.cpp'
assert not new.exists()
source = old.read_text()
source = source.replace('save_problem_json(trial,folder/("problem_h"+std::to_string(hires)+".json"));',
    'save_problem_json(trial,folder/("problem_h"+std::to_string(hires)+".json"));\n            save_actions(orders,folder/("orders_h"+std::to_string(hires)+".txt"));')
source = source.replace('                auto actions=*solved.schedule;', '''                const auto certificate=replay_schedule(trial,*solved.schedule);
                if(!certificate.requirements_satisfied || !certificate.invariants_satisfied)std::abort();
                save_actions(*solved.schedule,folder/("raw_schedule_h"+std::to_string(hires)+".txt"));
                auto actions=*solved.schedule;''')
source = source.replace('                // The full live-opponent day', '''                save_actions(actions,folder/("executable_h"+std::to_string(hires)+".txt"));
                // The full live-opponent day''')
source = source.replace('                if(!equal) {\n                    std::ofstream witness', '''                if(!equal) {
                    std::ofstream tiles(folder/("tiles_h"+std::to_string(hires)+".txt"));
                    for(int cell=0;cell<100;++cell){
                        const auto a=managed(endpoint.st.farms[0].tiles[cell/10][cell%10],day+1);
                        const auto b=*trial.required_end_tiles[cell].exact_state;
                        if(a!=b){
                            tiles<<cell<<" actual feed/care/dry/bank/yield "<<a.fed_today<<' '<<a.cared_today<<' '<<a.consecutive_dry_days<<' '<<a.pending_care_bonus<<' '<<a.stored_units
                                 <<" expected "<<b.fed_today<<' '<<b.cared_today<<' '<<b.consecutive_dry_days<<' '<<b.pending_care_bonus<<' '<<b.stored_units<<'\\n';
                        }
                    }
                    auto trace=saved;auto trace_rival=saved_rival;
                    std::ofstream steps(folder/("trace_h"+std::to_string(hires)+".csv"));
                    steps<<"hour,unit,cell,operation,arg,quantity,wheat_before,shed_before,shed_after,fed_before,fed_after,care_before,care_after\\n";
                    for(int h=0;h<24 && !trace.st.done;++h){
                        std::copy_n(season.shops.begin(),trace.st.n_shops,trace.st.shops);
                        Action other;trace_rival.act(agent::runtime::make_observation(trace,1),decision_budget(),other);
                        const auto before=trace;trace.step(actions[h],other);
                        for(int u=0;u<before.st.farms[0].n_units;++u){
                            const auto& f=before.st.farms[0];const int cell=f.pos_y[u]*10+f.pos_x[u];
                            const auto& a=f.tiles[cell/10][cell%10];const auto& b=trace.st.farms[0].tiles[cell/10][cell%10];
                            const auto& op=actions[h].units[u];
                            if(op.op==OP_FEED || op.op==OP_PICKUP || op.op==OP_PLACE || op.op==OP_DROP)
                                steps<<h<<','<<u<<','<<cell<<','<<+op.op<<','<<+op.arg<<','<<op.n<<','<<f.inv[u][WHEAT]<<','<<f.shed[WHEAT]<<','<<trace.st.farms[0].shed[WHEAT]<<','<<a.fed_today<<','<<b.fed_today<<','<<a.cared_today<<','<<b.cared_today<<'\\n';
                        }
                    }
                    std::ofstream witness''')
new.write_text(source)
cmake = RUN/'CMakeLists.txt'
text = cmake.read_text()
assert text.count('foreach(target estimate compile)') == 1
cmake.write_text(text.replace('foreach(target estimate compile)','foreach(target estimate compile compile_v2)'))
(RUN/'DIAGNOSTIC_COMPILER.json').write_text(json.dumps({'original':str(old.relative_to(RUN)),
    'original_sha256':hashlib.sha256(old.read_bytes()).hexdigest(),
    'new_sha256':hashlib.sha256(new.read_bytes()).hexdigest(),
    'changes':['Explicit raw physical replay certificate.','Save raw and executable schedules and gross orders.',
        'On full-game failure save tile differences and all feed/pickup/deposit transitions.'],
    'policy_or_task_change':False},indent=2)+'\n')
